/*
 ***************************************************************************
 *  File: artifact.c                                         Part of Duris *
 *  Usage: routines for artifact-tracking system                           *
 *  This is a new file for artifacts using the mySQL database              *
 *  For old code, see artifact_old.c                                       *
 *  Copyright ??? - Duris Systems Ltd.                                     *
 ***************************************************************************
 */

#include "prototypes.h"
#include "structs.h"
#include "artifact_cache_codec.h"
#include "comm.h"
#include "db.h"
#include "utility.h"
#include "utils.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include "files.h"
#include "flatfile/flatfile_artifact_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "mm.h"
#include "necromancy.h"
#include "persistence_mode.h"
#include "redis/redis_report_cache.h"
#include "spells.h"
#include "sql.h"
#include "vnum.obj.h"

#ifndef __NO_MYSQL__
#include <cjson/cJSON.h>
#endif

// Artifact types.
#define ARTIFACT_MAJOR 1
#define ARTIFACT_UNIQUE 2
#define ARTIFACT_IOUN 3

// Artifact locations.  These values match the DB locType INT column.
#define ARTIFACT_NOTINGAME 1
#define ARTIFACT_ON_NPC 2
#define ARTIFACT_ON_PC 3
#define ARTIFACT_ONGROUND 4
#define ARTIFACT_ONCORPSE 5 // PC corpse is implied here.

// Externals
extern P_room world;
extern const int top_of_world;
extern P_index obj_index;
extern P_index mob_index;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_obj object_list;
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;

// Internal globals
bool updateArtis = TRUE;
constexpr int ARTIFACT_MAINTENANCE_RETRY_DELAY = 30 * WAIT_SEC;
constexpr size_t ARTIFACT_BIND_BATCH_SIZE = 8;
constexpr size_t ARTIFACT_EXPIRY_BATCH_SIZE = 1;
constexpr size_t ARTIFACT_WARS_OWNER_BATCH_SIZE = 4;

// forward declarations for redis cache
P_char load_dummy_char(char *name);
void nuke_eq(P_char ch);
void arti_redis_cache(int type, bool Godlist);

// invalidate redis cache
static void arti_cache_invalidate(void)
{
	redis_invalidate_artifact_cache();
}

static void artifact_bind_maintenance_update(int vnum, int owner_pid, long timer)
{
#ifdef __NO_MYSQL__
	std::string error;
	const auto result = flatfile_artifact_bind_update(persistence_mode_flatfile_root(), vnum,
							  owner_pid, timer, &error);
	if (result != flatfile_artifact_result::ok && result != flatfile_artifact_result::unchanged)
	{
		logit(LOG_ARTIFACT,
		      "artifact binding maintenance could not update flat authority for %d: %s",
		      vnum, error.empty() ? "unknown error" : error.c_str());
	}
#else
	qry("UPDATE artifact_bind SET owner_pid = %d, timer = %ld WHERE vnum = %d", owner_pid,
	    timer, vnum);
#endif
}

// populate redis cache at boot
void arti_cache_init(void)
{
	if (!redis_report_cache_enabled())
		return;

	int t;
	for (t = 1; t <= 3; t++)
	{
		arti_redis_cache(t, FALSE);
		arti_redis_cache(t, TRUE);
	}
}

#ifndef __NO_MYSQL__
// json for redis/website
static char *arti_generate_json(int type, bool Godlist)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	P_obj obj;
	P_char owner = NULL;
	char *locName;
	int racewar;
	cJSON *root, *arr, *item;

	if (Godlist)
		qry("SELECT vnum, locType, location, owned, UNIX_TIMESTAMP(timer), lastUpdate FROM artifacts WHERE type=%d",
		    type);
	else
		qry("SELECT vnum, locType, location, owned FROM artifacts_mortal WHERE type=%d",
		    type);

	res = mysql_store_result(DB);
	if (!res)
		return NULL;

	root = cJSON_CreateObject();
	arr = cJSON_CreateArray();
	if (!root || !arr)
	{
		if (root)
			cJSON_Delete(root);
		if (arr)
			cJSON_Delete(arr);
		mysql_free_result(res);
		return NULL;
	}
	cJSON_AddItemToObject(root, "artifacts", arr);
	cJSON_AddNumberToObject(root, "schema_version", ARTIFACT_CACHE_SCHEMA_VERSION);
	cJSON_AddNumberToObject(root, "type", type);
	cJSON_AddBoolToObject(root, "godlist", Godlist);

	int articount[5] = { 0 };

	while ((row = mysql_fetch_row(res)))
	{
		int vnum = atoi(row[0]);
		int locType = atoi(row[1]);
		int location = atoi(row[2]);
		bool owned = (row[3][0] == 'Y');

		obj = read_object(vnum, VIRTUAL);
		if (!obj || !IS_ARTIFACT(obj))
		{
			if (obj)
				extract_obj(obj, FALSE);
			continue;
		}

		item = cJSON_CreateObject();
		if (!item)
		{
			extract_obj(obj, FALSE);
			mysql_free_result(res);
			cJSON_Delete(root);
			return NULL;
		}
		cJSON_AddNumberToObject(item, "vnum", vnum);
		cJSON_AddNumberToObject(item, "locType", locType);
		cJSON_AddNumberToObject(item, "location", location);
		cJSON_AddBoolToObject(item, "owned", owned);
		cJSON_AddStringToObject(item, "shortDesc", obj->short_description);

		if (Godlist)
		{
			cJSON_AddNumberToObject(item, "timer", row[4] ? atol(row[4]) : 0);
			cJSON_AddStringToObject(item, "lastUpdate", row[5] ? row[5] : "");
		}

		racewar = RACEWAR_NONE;
		if (locType == ARTIFACT_ON_PC || locType == ARTIFACT_ONCORPSE)
		{
			locName = get_player_name_from_pid(location);
			if (locName)
			{
				cJSON_AddStringToObject(item, "ownerName", locName);
				owner = load_dummy_char(locName);
				if (owner)
				{
					racewar = GET_RACEWAR(owner);
					nuke_eq(owner);
					owner->in_room = NOWHERE;
					extract_char(owner);
				}
			}
		}
		cJSON_AddNumberToObject(item, "racewar", racewar);

		if (owned && (locType == ARTIFACT_ON_PC || locType == ARTIFACT_ONCORPSE))
		{
			articount[RACEWAR_NONE]++;
			if (racewar != RACEWAR_NONE)
				articount[racewar]++;
		}

		cJSON_AddItemToArray(arr, item);
		extract_obj(obj, FALSE);
	}

	mysql_free_result(res);

	cJSON *summary = cJSON_CreateObject();
	if (!summary)
	{
		cJSON_Delete(root);
		return NULL;
	}
	cJSON_AddNumberToObject(summary, "total", articount[RACEWAR_NONE]);
	cJSON_AddNumberToObject(summary, "good", articount[RACEWAR_GOOD]);
	cJSON_AddNumberToObject(summary, "evil", articount[RACEWAR_EVIL]);
	cJSON_AddItemToObject(root, "summary", summary);

	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return json;
}

void arti_redis_cache(int type, bool Godlist)
{
	if (!redis_report_cache_enabled())
		return;

	char *json = arti_generate_json(type, Godlist);
	if (!json)
		return;

	redis_cache_artifact_list(type, Godlist, json);
	free(json);
}
#else
void arti_redis_cache(int /*type*/, bool /*Godlist*/) {}
#endif

// forward declarations
void list_artifacts_sql(P_char ch, int type, bool Godlist, bool allArtis);
void arti_clear_sql(P_char ch, char *arg);
void arti_files_to_sql(P_char ch, char *arg);
void arti_hunt_sql(P_char ch, const char *arg);
void arti_player_sql(P_char ch, char *arg);
void arti_poof_sql(P_char ch, char *arg);
void arti_remove_sql(int vnum, bool mortalToo);
void arti_reset_sql(P_char ch, char *arg);
void arti_swap_sql(P_char ch, char *arg);
void arti_syncdb_sql(P_char ch);
void arti_timer_sql(P_char ch, char *arg);
void artifact_update_sql(P_obj arti, char owned, time_t timer);
P_char load_dummy_char(char *name);
void nuke_eq(P_char ch);

/* This is an example of what the current artifacts table looks like. - 2/23/2015
+------+-------+-----------+----------+---------------------+------+---------------------+
| vnum | owned | locType   | location | timer               | type | lastUpdate          |
+------+-------+-----------+----------+---------------------+------+---------------------+
|  900 | N     | OnNPC     |    81454 | 2015-02-28 02:03:05 |    3 | 2015-06-29 03:28:46 |
|  901 | N     | NotInGame |        0 | 0000-00-00 00:00:00 |    3 | 2015-06-29 03:28:46 |
|  902 | Y     | OnPC      |    26636 | 2015-02-28 02:03:05 |    3 | 2015-06-29 03:28:46 |
|  903 | Y     | OnGround  |     1200 | 2015-02-28 02:03:05 |    3 | 2015-06-29 03:28:46 |
|  904 | Y     | OnCorpse  |    26636 | 2015-02-28 02:03:05 |    3 | 2015-06-29 03:28:46 |
+------+-------+-----------+----------+---------------------+------+---------------------+
          vnum      = vnum of the artifact
          owned     = 'Y' -> artifat is owned, 'N' artifact has yet to be acquired since last poof.
                      If the artifact is owned, then it's timer is set and ticking (sorta).
          locType   = 1=NotInGame, 2=OnNPC, 3=OnPC, 4=OnGround, 5=OnCorpse (OnCorpse -> PC Corpse)
                    = ARTIFACT_ {NOTINGAME | ON_NPC | ON_PC | ONGROUND | ONCORPSE}
          location  = PID of PC / vnum of NPC / vnum of room.
          timer     = when the arti is due to poof.
          type      = full artifact -> ARTIFACT_MAJOR, unique -> ARTIFACT_UNIQUE, ioun -> ARTIFACT_IOUN
          lastUpdate= last time this entry was updated.
*/

// This function handles the input and routes to the correct function.
//   Just sends a list of possible arguments to ch if arg is not valid input.
void do_artifact_sql(P_char ch, char *arg, int /*cmd*/)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
	char *rest;
	bool allArtis, Godlist;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	arg = one_argument(arg, arg1);
	rest = one_argument(arg, arg2);
	rest = one_argument(rest, arg3);
	arg = skip_spaces(arg);

	// all -> show even the artis not in game.
	if (IS_TRUSTED(ch) &&
	    ((*arg3 && is_abbrev(arg3, "all")) || (*arg2 && is_abbrev(arg2, "all"))))
	{
		allArtis = TRUE;
	}
	else
	{
		allArtis = FALSE;
	}

	if (!IS_TRUSTED(ch) ||
	    ((*arg3 && is_abbrev(arg3, "mortal")) || (*arg2 && is_abbrev(arg2, "mortal"))))
	{
		Godlist = FALSE;
	}
	else
	{
		Godlist = TRUE;
	}

	if (is_abbrev(arg1, "list") || is_abbrev(arg1, "major"))
	{
		list_artifacts_sql(ch, ARTIFACT_MAJOR, Godlist, allArtis);
		return;
	}

	if (is_abbrev(arg1, "unique"))
	{
		list_artifacts_sql(ch, ARTIFACT_UNIQUE, Godlist, allArtis);
		return;
	}

	if (is_abbrev(arg1, "ioun"))
	{
		list_artifacts_sql(ch, ARTIFACT_IOUN, Godlist, allArtis);
		return;
	}

	if (GET_LEVEL(ch) < FORGER || IS_NPC(ch))
	{
		send_to_char("Valid arguments are major, unique or ioun.\n\r", ch);
		return;
	}

	// At this point, arg holds all the arguments but arg1.
	if (is_abbrev(arg1, "player"))
	{
		arti_player_sql(ch, arg);
		return;
	}

	if (is_abbrev(arg1, "poof"))
	{
		arti_poof_sql(ch, arg);
		return;
	}

	if (is_abbrev(arg1, "swap"))
	{
		arti_swap_sql(ch, arg);
		return;
	}

	if (is_abbrev(arg1, "timer"))
	{
		arti_timer_sql(ch, arg);
		return;
	}

	if (is_abbrev(arg1, "hunt"))
	{
		arti_hunt_sql(ch, arg);
		return;
	}

	if (is_abbrev(arg1, "clear"))
	{
		arti_clear_sql(ch, arg);
		return;
	}

	if (is_abbrev(arg1, "files"))
	{
		arti_files_to_sql(ch, arg);
		return;
	}

	if (is_abbrev(arg1, "reset"))
	{
		arti_reset_sql(ch, arg);
		return;
	}

	send_to_char(
		"Valid arguments are major, unique, ioun, swap, poof, timer, hunt, clear, or files.\n\r",
		ch);
	send_to_char(
		"Valid sub-arguments for list, unique, ioun are [mortal] - shows mortal list and [all] shows un-owned artis.\n\r",
		ch);
}

// display artifact list from redis cache
void list_artifacts_sql(P_char ch, int type, bool Godlist, bool allArtis)
{
#ifndef __NO_MYSQL__
	char buf[MAX_STRING_LENGTH];
	char *json;
	cJSON *root, *artifacts, *item;
	int articount[5] = { 0 };
	bool shownData = FALSE;

	if (type != ARTIFACT_MAJOR && type != ARTIFACT_UNIQUE && type != ARTIFACT_IOUN)
	{
		send_to_char("Invalid artifact type.\n\r", ch);
		return;
	}

	// Treat cache unavailability or malformed data as a miss. The SQL-generated
	// payload is rendered directly; cache publication is best effort.
	root = NULL;
	if (redis_report_cache_enabled())
	{
		json = redis_get_artifact_list(type, Godlist);
		if (json)
		{
			root = cJSON_Parse(json);
			free(json);
			if (!artifact_cache_payload_valid(root, type, Godlist))
			{
				if (root)
					cJSON_Delete(root);
				root = NULL;
				redis_invalidate_artifact_list(type, Godlist);
				logit(LOG_SYS,
				      "redis: rejected malformed artifact cache type=%d godlist=%d",
				      type, Godlist ? 1 : 0);
			}
		}
	}
	if (!root)
	{
		json = arti_generate_json(type, Godlist);
		if (!json)
		{
			send_to_char("Artifact data is temporarily unavailable.\n\r", ch);
			return;
		}
		root = cJSON_Parse(json);
		if (!artifact_cache_payload_valid(root, type, Godlist))
		{
			free(json);
			if (root)
				cJSON_Delete(root);
			logit(LOG_SYS,
			      "artifact: generated invalid list payload type=%d godlist=%d", type,
			      Godlist ? 1 : 0);
			send_to_char("Artifact data is temporarily unavailable.\n\r", ch);
			return;
		}
		if (redis_report_cache_enabled())
			redis_cache_artifact_list(type, Godlist, json);
		free(json);
	}

	artifacts = cJSON_GetObjectItem(root, "artifacts");

	// header
	if (Godlist)
		snprintf(buf, MAX_STRING_LENGTH,
			 "&+YOwner                  Time      Last Update           %s\r\n\r\n",
			 type == ARTIFACT_MAJOR	 ? "Artifact" :
			 type == ARTIFACT_UNIQUE ? "Unique" :
						   "Ioun");
	else
		snprintf(buf, MAX_STRING_LENGTH, "&+YOwner               %s\r\n\r\n",
			 type == ARTIFACT_MAJOR	 ? "Artifact" :
			 type == ARTIFACT_UNIQUE ? "Unique" :
						   "Ioun");
	send_to_char(buf, ch);

	cJSON_ArrayForEach(item, artifacts)
	{
		int vnum = cJSON_GetObjectItem(item, "vnum")->valueint;
		int locType = cJSON_GetObjectItem(item, "locType")->valueint;
		int location = cJSON_GetObjectItem(item, "location")->valueint;
		bool owned = cJSON_IsTrue(cJSON_GetObjectItem(item, "owned"));
		const char *shortDesc = cJSON_GetObjectItem(item, "shortDesc")->valuestring;
		int racewar = cJSON_GetObjectItem(item, "racewar")->valueint;

		if (!allArtis && !owned)
			continue;

		// mortal view only shows pc/corpse
		if (!Godlist && !(locType == ARTIFACT_ON_PC || locType == ARTIFACT_ONCORPSE))
			continue;

		// get owner name from json
		char *locName = NULL;
		char locNameBuf[MAX_STRING_LENGTH];
		cJSON *ownerNameItem = cJSON_GetObjectItem(item, "ownerName");

		switch (locType)
		{
		case ARTIFACT_NOTINGAME:
			locName = (char *)"&+RNotInGame&n";
			break;
		case ARTIFACT_ON_NPC:
			locName = (char *)"&+YOnMob&n";
			break;
		case ARTIFACT_ON_PC:
			if (ownerNameItem && ownerNameItem->valuestring)
				locName = ownerNameItem->valuestring;
			break;
		case ARTIFACT_ONGROUND:
			snprintf(locNameBuf, MAX_STRING_LENGTH, "Room #%d", location);
			locName = locNameBuf;
			break;
		case ARTIFACT_ONCORPSE:
			if (ownerNameItem && ownerNameItem->valuestring)
			{
				if (Godlist)
					snprintf(locNameBuf, MAX_STRING_LENGTH, "%s's corpse",
						 ownerNameItem->valuestring);
				else
					snprintf(locNameBuf, MAX_STRING_LENGTH, "%s",
						 ownerNameItem->valuestring);
				locName = locNameBuf;
			}
			break;
		default:
			continue;
		}

		if (!locName)
			locName = (char *)"&+RUnknown&n";

		// count for summary
		if (owned && (locType == ARTIFACT_ON_PC || locType == ARTIFACT_ONCORPSE))
		{
			articount[RACEWAR_NONE]++;
			if (racewar != RACEWAR_NONE)
				articount[racewar]++;
		}

		if (!Godlist)
		{
			checked_snprintf(buf, MAX_STRING_LENGTH, "%-20s%s\r\n", locName, shortDesc);
			send_to_char(buf, ch);
			shownData = TRUE;
			continue;
		}

		// calc TIME fresh from raw timestamp
		cJSON *timerItem = cJSON_GetObjectItem(item, "timer");
		long timer = timerItem ? (long)timerItem->valuedouble : 0;
		long totalTime = timer - time(NULL);
		bool negTime = FALSE;
		if (totalTime < 0)
		{
			negTime = TRUE;
			totalTime *= -1;
		}
		totalTime /= 60;
		int minutes = totalTime % 60;
		totalTime /= 60;
		int hours = totalTime % 24;
		int days = totalTime / 24;
		if (timer == 0)
			days = hours = minutes = 0;

		char timerBuf[32];
		snprintf(timerBuf, 32, "%c%2d:%02d:%02d", negTime ? '-' : ' ', days, hours,
			 minutes);

		cJSON *lastUpdateItem = cJSON_GetObjectItem(item, "lastUpdate");
		const char *lastUpdate = lastUpdateItem ? lastUpdateItem->valuestring : "";

		char locPadded[MAX_STRING_LENGTH];
		snprintf(locPadded, MAX_STRING_LENGTH, "%s",
			 pad_ansi(locName, MAX_NAME_LENGTH + 9, TRUE).c_str());
		checked_snprintf(buf, MAX_STRING_LENGTH, "%-21s&n%-11s %-22s%s (#%d)\r\n",
				 locPadded, timerBuf, lastUpdate, shortDesc, vnum);
		send_to_char(buf, ch);
		shownData = TRUE;
	}

	cJSON_Delete(root);

	if (!shownData)
	{
		send_to_char("No artifacts found.\r\n", ch);
		return;
	}

	// summary
	snprintf(buf, MAX_STRING_LENGTH, "\r\n       &+r------&+LSummary&+r------&n\r\n");
	checked_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf),
			 "         &+WGoodies:      %d&n\r\n", articount[RACEWAR_GOOD]);
	checked_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf),
			 "         &+rEvils:        %d&n\r\n", articount[RACEWAR_EVIL]);
	if (articount[RACEWAR_UNDEAD])
		checked_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf),
				 "         &+LUndead:       %d&n\r\n", articount[RACEWAR_UNDEAD]);
	if (articount[RACEWAR_NEUTRAL])
		checked_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf),
				 "         &+MNeutral:      %d&n\r\n", articount[RACEWAR_NEUTRAL]);
	checked_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf),
			 "         &+WTotal:        %d\r\n", articount[RACEWAR_NONE]);
	send_to_char(buf, ch);
#else
	char buf[MAX_STRING_LENGTH];
	int articount[5] = { 0 };
	bool shownData = FALSE;
	if (type != ARTIFACT_MAJOR && type != ARTIFACT_UNIQUE && type != ARTIFACT_IOUN)
	{
		send_to_char("Invalid artifact type.\n\r", ch);
		return;
	}
	std::vector<flatfile_artifact_record> records;
	std::string error;
	if (flatfile_artifact_list(persistence_mode_flatfile_root(), &records, &error) !=
	    flatfile_artifact_result::ok)
	{
		logit(LOG_ARTIFACT, "list_artifacts_sql: flat artifact read failed: %s",
		      error.empty() ? "missing or invalid artifact authority" : error.c_str());
		send_to_char("Artifact data is unavailable.\n\r", ch);
		return;
	}
	if (Godlist)
		snprintf(buf, MAX_STRING_LENGTH,
			 "&+YOwner                  Time      Last Update           %s\r\n\r\n",
			 type == ARTIFACT_MAJOR	 ? "Artifact" :
			 type == ARTIFACT_UNIQUE ? "Unique" :
						   "Ioun");
	else
		snprintf(buf, MAX_STRING_LENGTH, "&+YOwner               %s\r\n\r\n",
			 type == ARTIFACT_MAJOR	 ? "Artifact" :
			 type == ARTIFACT_UNIQUE ? "Unique" :
						   "Ioun");
	send_to_char(buf, ch);

	for (const auto &record : records)
	{
		if (record.type != type || (!allArtis && !record.owned) ||
		    (!Godlist && record.location_type != ARTIFACT_ON_PC &&
		     record.location_type != ARTIFACT_ONCORPSE))
			continue;
		P_obj artifact = read_object(record.vnum, VIRTUAL);
		if (!artifact || !IS_ARTIFACT(artifact))
		{
			if (artifact)
				extract_obj(artifact, FALSE);
			continue;
		}
		char *owner_name = NULL;
		int racewar = RACEWAR_NONE;
		if (record.location_type == ARTIFACT_ON_PC ||
		    record.location_type == ARTIFACT_ONCORPSE)
		{
			owner_name = get_player_name_from_pid(record.location);
			if (owner_name)
			{
				P_char owner = load_dummy_char(owner_name);
				if (owner)
				{
					racewar = GET_RACEWAR(owner);
					nuke_eq(owner);
					owner->in_room = NOWHERE;
					extract_char(owner);
				}
			}
		}
		char location_buffer[MAX_STRING_LENGTH];
		const char *location_name = NULL;
		switch (record.location_type)
		{
		case ARTIFACT_NOTINGAME:
			location_name = "&+RNotInGame&n";
			break;
		case ARTIFACT_ON_NPC:
			location_name = "&+YOnMob&n";
			break;
		case ARTIFACT_ON_PC:
			location_name = owner_name;
			break;
		case ARTIFACT_ONGROUND:
			snprintf(location_buffer, sizeof(location_buffer), "Room #%d",
				 record.location);
			location_name = location_buffer;
			break;
		case ARTIFACT_ONCORPSE:
			if (owner_name)
			{
				snprintf(location_buffer, sizeof(location_buffer),
					 Godlist ? "%s's corpse" : "%s", owner_name);
				location_name = location_buffer;
			}
			break;
		default:
			extract_obj(artifact, FALSE);
			continue;
		}
		if (!location_name)
			location_name = "&+RUnknown&n";
		if (record.owned && (record.location_type == ARTIFACT_ON_PC ||
				     record.location_type == ARTIFACT_ONCORPSE))
		{
			++articount[RACEWAR_NONE];
			if (racewar > RACEWAR_NONE && racewar <= RACEWAR_NEUTRAL)
				++articount[racewar];
		}
		if (!Godlist)
		{
			checked_snprintf(buf, MAX_STRING_LENGTH, "%-20s%s\r\n", location_name,
					 artifact->short_description);
			send_to_char(buf, ch);
			shownData = TRUE;
			extract_obj(artifact, FALSE);
			continue;
		}
		long total_time = static_cast<long>(record.timer - time(NULL));
		bool negative_time = total_time < 0;
		if (negative_time)
			total_time *= -1;
		total_time /= 60;
		const int minutes = total_time % 60;
		total_time /= 60;
		const int hours = total_time % 24;
		long days = total_time / 24;
		if (!record.timer)
		{
			negative_time = FALSE;
			days = 0;
		}
		char timer_buffer[32];
		snprintf(timer_buffer, sizeof(timer_buffer), "%c%2ld:%02d:%02d",
			 negative_time ? '-' : ' ', days, record.timer ? hours : 0,
			 record.timer ? minutes : 0);
		char update_buffer[32] = "";
		const time_t updated = static_cast<time_t>(record.last_update);
		struct tm update_time;
		if (record.last_update > 0 && localtime_r(&updated, &update_time))
			strftime(update_buffer, sizeof(update_buffer), "%Y-%m-%d %H:%M:%S",
				 &update_time);
		char padded_location[MAX_STRING_LENGTH];
		snprintf(padded_location, sizeof(padded_location), "%s",
			 pad_ansi(location_name, MAX_NAME_LENGTH + 9, TRUE).c_str());
		checked_snprintf(buf, MAX_STRING_LENGTH, "%-21s&n%-11s %-22s%s (#%d)\r\n",
				 padded_location, timer_buffer, update_buffer,
				 artifact->short_description, record.vnum);
		send_to_char(buf, ch);
		shownData = TRUE;
		extract_obj(artifact, FALSE);
	}
	if (!shownData)
	{
		send_to_char("No artifacts found.\r\n", ch);
		return;
	}
	snprintf(buf, MAX_STRING_LENGTH, "\r\n       &+r------&+LSummary&+r------&n\r\n");
	checked_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf),
			 "         &+WGoodies:      %d&n\r\n", articount[RACEWAR_GOOD]);
	checked_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf),
			 "         &+rEvils:        %d&n\r\n", articount[RACEWAR_EVIL]);
	if (articount[RACEWAR_UNDEAD])
		checked_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf),
				 "         &+LUndead:       %d&n\r\n", articount[RACEWAR_UNDEAD]);
	if (articount[RACEWAR_NEUTRAL])
		checked_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf),
				 "         &+MNeutral:      %d&n\r\n", articount[RACEWAR_NEUTRAL]);
	checked_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf),
			 "         &+WTotal:        %d\r\n", articount[RACEWAR_NONE]);
	send_to_char(buf, ch);
#endif
}

// Remove artifact entry from the artifacts table.
//   mortalToo means that we remove the entry from the mortals' table too.
// This is used for removing an arti from the game (I guess).
void arti_remove_sql(int vnum, bool mortalToo)
{
	if (!updateArtis)
	{
		return;
	}

#ifdef __NO_MYSQL__
	(void)mortalToo;
	std::string error;
	const auto removed =
		flatfile_artifact_erase(persistence_mode_flatfile_root(), vnum, &error);
	if (removed != flatfile_artifact_result::ok &&
	    removed != flatfile_artifact_result::not_found)
		logit(LOG_ARTIFACT, "arti_remove_sql: flat artifact erase failed for %d: %s", vnum,
		      error.empty() ? "missing or invalid artifact authority" : error.c_str());
	else
		arti_cache_invalidate();
#else
	// Remove from artifacts table:
	qry("DELETE FROM artifacts WHERE vnum = '%d'", vnum);
	arti_cache_invalidate();
	// Possibly remove from artifacts_mortal table:
	if (mortalToo)
	{
		qry("DELETE FROM artifacts_mortal WHERE vnum = '%d'", vnum);
	}
#endif
}

// This function is called at boot to set the mortals' artifact list table.
void setupMortArtiList_sql()
{
#ifdef __NO_MYSQL__
	std::vector<flatfile_artifact_record> records;
	std::string error;
	if (flatfile_artifact_list(persistence_mode_flatfile_root(), &records, &error) !=
	    flatfile_artifact_result::ok)
	{
		logit(LOG_ARTIFACT, "setupMortArtiList_sql: flat artifact read failed: %s",
		      error.empty() ? "missing or invalid artifact authority" : error.c_str());
		return;
	}
#else
	// Clear the mortals table.
	qry("TRUNCATE TABLE artifacts_mortal");
	// Arih : Explicitly specify columns to avoid "Column count doesn't match value count" error.
	// artifacts_mortal doesn't have 'lastUpdate' column but artifacts does, so SELECT * fails.
	// Repopulate it: Only select columns that exist in both tables (excluding lastUpdate)
	qry("INSERT INTO artifacts_mortal (vnum, owned, locType, location, timer, type) SELECT vnum, owned, locType, location, timer, type FROM artifacts WHERE locType=%d OR locType=%d",
	    ARTIFACT_ON_PC, ARTIFACT_ONCORPSE);
#endif

	arti_cache_init();
}

// Loads the artis that were on the ground and owned back into the boot.
void addOnGroundArtis_sql()
{
	P_obj arti;
	int room;
#ifndef __NO_MYSQL__
	MYSQL_RES *res;
	MYSQL_ROW row;
#endif

	logit(LOG_ARTIFACT, "addOnGroundArtis_sql: Beginning.");

#ifdef __NO_MYSQL__
	std::vector<flatfile_artifact_record> records;
	std::string error;
	if (flatfile_artifact_list(persistence_mode_flatfile_root(), &records, &error) !=
	    flatfile_artifact_result::ok)
	{
		logit(LOG_ARTIFACT, "addOnGroundArtis_sql: flat artifact read failed: %s",
		      error.empty() ? "missing or invalid artifact authority" : error.c_str());
		return;
	}
	bool found = false;
	for (const auto &record : records)
	{
		if (!record.owned || record.location_type != ARTIFACT_ONGROUND)
			continue;
		found = true;
		if (!(arti = read_object(record.vnum, VIRTUAL)))
		{
			logit(LOG_ARTIFACT, "addOnGroundArtis_sql: Could not load object vnum %d.",
			      record.vnum);
			continue;
		}
		if ((room = real_room(record.location)) < 0 || room > top_of_world)
		{
			logit(LOG_ARTIFACT, "addOnGroundArtis_sql: Could not find room %d.",
			      record.location);
			extract_obj(arti, FALSE);
			continue;
		}
		obj_to_room(arti, room);
	}
	if (!found)
		logit(LOG_ARTIFACT, "addOnGroundArtis_sql: No owned artifacts found on ground.");
#else
	qry("SELECT vnum, location FROM artifacts WHERE owned='Y' AND locType=%d",
	    ARTIFACT_ONGROUND);

	if ((res = mysql_store_result(DB)) != NULL)
	{
		if (mysql_num_rows(res) < 1)
		{
			logit(LOG_ARTIFACT,
			      "addOnGroundArtis_sql: No owned artifacts found on ground.");
		}
		else
		{
			while ((row = mysql_fetch_row(res)))
			{
				if (!(arti = read_object(atoi(row[0]), VIRTUAL)))
				{
					logit(LOG_ARTIFACT,
					      "addOnGroundArtis_sql: Could not load object vnum %d.",
					      atoi(row[0]));
					continue;
				}
				if ((room = real_room(atoi(row[1]))) < 0 || room > top_of_world)
				{
					logit(LOG_ARTIFACT,
					      "addOnGroundArtis_sql: Could not find room %d.",
					      atoi(row[1]));
					extract_obj(arti, FALSE);
					continue;
				}
				obj_to_room(arti, room);
			}
		}
		mysql_free_result(res);
	}
	else
	{
		logit(LOG_ARTIFACT, "addOnGroundArtis_sql: Could not pull on ground arti list.");
	}
#endif

	logit(LOG_ARTIFACT, "addOnGroundArtis_sql: Ending.");
}

// This function either finds the row in the artifacts table corresponding to arti,
//   or makes one, if there isn't one already.  If there is one, it calculates
//   the new time to poof.
void artifact_feed_to_min_sql(P_obj arti, int min_minutes)
{
	int vnum = OBJ_VNUM(arti);
	long unsigned to_time;
	P_char owner;
	P_obj cont;
#ifndef __NO_MYSQL__
	int location;
	long unsigned oldtime;
	MYSQL_RES *res;
	MYSQL_ROW row;
#endif

	if (!updateArtis)
	{
		return;
	}

	// Don't be corruptin' my table with a non-artifact!
	if (!IS_ARTIFACT(arti))
	{
		logit(LOG_ARTIFACT, "artifact_feed_to_min_sql: Non arti vnum %d.", vnum);
		return;
	}

	// Calculate the minimum time to poof in seconds.
	to_time = time(NULL) + min_minutes * 60;

	// Arih : Validate to_time to prevent MySQL error "Incorrect datetime value: '1970-01-01 00:00:00'".
	// When min_minutes is 0 or negative, FROM_UNIXTIME(0) causes MySQL to reject the datetime.
	// Ensure to_time is never 0 or in the past (MySQL will reject FROM_UNIXTIME(0))
	if (to_time <= 0)
	{
		to_time = time(NULL) +
			  ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY; // 10 days, not 60 secs
		logit(LOG_ARTIFACT,
		      "artifact_feed_to_min_sql: WARNING: to_time was %ld, resetting to current time + 10 days for vnum %d",
		      (long)(time(NULL) + min_minutes * 60), vnum);
	}

#ifdef __NO_MYSQL__
	flatfile_artifact_record record;
	std::string error;
	const auto loaded =
		flatfile_artifact_get(persistence_mode_flatfile_root(), vnum, &record, &error);
	if (loaded == flatfile_artifact_result::ok)
	{
		if (!record.owned)
			logit(LOG_ARTIFACT,
			      "artifact_feed_to_min_sql: WARNING: Updating time on non-owned artifact %d.",
			      vnum);
		const auto extended = flatfile_artifact_extend_timer(
			persistence_mode_flatfile_root(), vnum, to_time, time(NULL), &error);
		if (extended != flatfile_artifact_result::ok &&
		    extended != flatfile_artifact_result::unchanged)
			logit(LOG_ARTIFACT,
			      "artifact_feed_to_min_sql: flat timer extension failed for %d: %s",
			      vnum, error.empty() ? "invalid artifact authority" : error.c_str());
		else
			arti_cache_invalidate();
		return;
	}
	if (loaded != flatfile_artifact_result::not_found)
	{
		logit(LOG_ARTIFACT, "artifact_feed_to_min_sql: flat artifact read failed: %s",
		      error.empty() ? "invalid artifact authority" : error.c_str());
		return;
	}
	cont = arti;
	if (OBJ_INSIDE(cont))
	{
		logit(LOG_ARTIFACT,
		      "artifact_feed_to_min_sql: arti vnum %d is inside a container?!", vnum);
		while (OBJ_INSIDE(cont) && cont->loc.inside)
			cont = cont->loc.inside;
	}
	if (OBJ_ROOM(cont))
		artifact_update_sql(arti, 'Y', to_time);
	else if (OBJ_WORN(cont) || OBJ_CARRIED(cont))
	{
		owner = OBJ_WORN(cont) ? cont->loc.wearing : cont->loc.carrying;
		if (!owner)
			logit(LOG_ARTIFACT,
			      "artifact_feed_to_min_sql: arti vnum %d worn or carried, but no owner?!",
			      vnum);
		else
			artifact_update_sql(arti, IS_NPC(owner) ? 'N' : 'Y', to_time);
	}
	else if (OBJ_INSIDE(cont))
		logit(LOG_ARTIFACT,
		      "artifact_feed_to_min_sql: arti vnum %d is inside a non-existent container?!",
		      vnum);
	else if (OBJ_NOWHERE(cont))
		logit(LOG_ARTIFACT,
		      "artifact_feed_to_min_sql: arti vnum %d is in location NOWHERE?!", vnum);
	else
		logit(LOG_ARTIFACT,
		      "artifact_feed_to_min_sql: arti vnum %d is in an UNKNOWN location?!", vnum);
	return;
#else
	if (!qry("select owned, UNIX_TIMESTAMP(timer) from artifacts where vnum = %d", vnum))
	{
		logit(LOG_ARTIFACT, "artifact_feed_to_min_sql: failed to read from database.");
		return;
	}
	res = mysql_store_result(DB);
	if (!res)
	{
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		return;
	}
	if (mysql_num_rows(res) > 0)
	{
		if (!(row = mysql_fetch_row(res)))
		{
			logit(LOG_ARTIFACT, "artifact_feed_to_min_sql: failed to fetch row.");
			mysql_free_result(res);
			return;
		}
		if (strcmp(row[0], "Y"))
		{
			logit(LOG_ARTIFACT,
			      "artifact_feed_to_min_sql: WARNING: Updating time on non-owned (%s) artifact %d.",
			      row[0], vnum);
		}
		oldtime = atoi(row[1]);
		// Keep the bigger one, since we're feeding to at least min_minutes.
		to_time = (oldtime >= to_time) ? oldtime : to_time;

		qry("UPDATE artifacts SET timer = FROM_UNIXTIME(%lu), lastUpdate=SYSDATE() WHERE vnum = %d",
		    to_time, vnum);
		arti_cache_invalidate();
	}
	else
	{
		cont = arti;
		if (OBJ_INSIDE(cont))
		{
			logit(LOG_ARTIFACT,
			      "artifact_feed_to_min_sql: arti vnum %d is inside a container?!",
			      vnum);
			while (OBJ_INSIDE(cont) && cont->loc.inside)
			{
				cont = cont->loc.inside;
			}
		}

		if (OBJ_ROOM(cont))
		{
			// Take Rnum and convert to vnum.
			location = cont->loc.room;
			if (location < 0 || location > top_of_world)
			{
				// Converting room to 0 here 'cause we're putting it in The Void instead of out-of-bounds.
				location = 0;
			}
			else
			{
				location = world[location].number;
			}
			qry("INSERT INTO artifacts (vnum, owned, locType, location, timer, type, lastUpdate) VALUES(%d, 'Y', %d, %d, FROM_UNIXTIME(%lu), %d, SYSDATE() )",
			    vnum, ARTIFACT_ONGROUND, location, to_time,
			    IS_IOUN(arti)   ? ARTIFACT_IOUN :
			    IS_UNIQUE(arti) ? ARTIFACT_UNIQUE :
					      ARTIFACT_MAJOR);
			arti_cache_invalidate();
		}
		else if (OBJ_WORN(cont) || OBJ_CARRIED(cont))
		{
			owner = OBJ_WORN(cont) ? cont->loc.wearing : cont->loc.carrying;
			// We don't care if they're alive.
			if (!owner)
			{
				logit(LOG_ARTIFACT,
				      "artifact_feed_to_min_sql: arti vnum %d worn or carried, but no owner?!",
				      vnum);
			}
			else
			{
				// Adding a NPC owner to arti -> owned = 'N', location = mob vnum.
				if (IS_NPC(owner))
				{
					location = GET_VNUM(owner);
					qry("INSERT INTO artifacts (vnum, owned, locType, location, timer, type, lastUpdate) VALUES(%d, 'N', %d, %d, FROM_UNIXTIME(%lu), %d, SYSDATE() )",
					    vnum, ARTIFACT_ON_NPC, location, to_time,
					    IS_IOUN(arti)   ? ARTIFACT_IOUN :
					    IS_UNIQUE(arti) ? ARTIFACT_UNIQUE :
							      ARTIFACT_MAJOR);
					arti_cache_invalidate();
				}
				// Adding a PC owner to arti -> owned = 'Y', location = PID.
				else
				{
					location = GET_PID(owner);
					qry("INSERT INTO artifacts (vnum, owned, locType, location, timer, type, lastUpdate) VALUES(%d, 'Y', %d, %d, FROM_UNIXTIME(%lu), %d, SYSDATE() )",
					    vnum, ARTIFACT_ON_PC, location, to_time,
					    IS_IOUN(arti)   ? ARTIFACT_IOUN :
					    IS_UNIQUE(arti) ? ARTIFACT_UNIQUE :
							      ARTIFACT_MAJOR);
					arti_cache_invalidate();
				}
			}
		}
		else if (OBJ_INSIDE(cont))
		{
			logit(LOG_ARTIFACT,
			      "artifact_feed_to_min_sql: arti vnum %d is inside a non-existent container?!",
			      vnum);
		}
		else if (OBJ_NOWHERE(cont))
		{
			logit(LOG_ARTIFACT,
			      "artifact_feed_to_min_sql: arti vnum %d is in location NOWHERE?!",
			      vnum);
		}
		else
		{
			logit(LOG_ARTIFACT,
			      "artifact_feed_to_min_sql: arti vnum %d is in an UNKNOWN location?!",
			      vnum);
		}
	}
	mysql_free_result(res);
#endif
}

// This function handles the 'soul' of the artifact.
void artifact_switch_check(P_char ch, P_obj arti)
{
	int owner_pid, timer, vnum;
	bool update = FALSE;

	if (!updateArtis)
	{
		return;
	}

	// And make sure it's and artifact
	if (!IS_ARTIFACT(arti))
	{
		return;
	}

	// Gods don't affect the soul of the arti.
	if (IS_TRUSTED(ch))
	{
		return;
	}

	// Load up the variables.
	vnum = OBJ_VNUM(arti);
	if (!sql_get_bind_data(vnum, &owner_pid, &timer))
	{
		return;
	}

	// If a pvp loot happened, and timeframe has passed, set to 0 for binding
	if ((owner_pid == -1) &&
	    (timer + (60 * (int)get_property("artifact.feeding.switch.lootallowance.min", 30)) <
	     time(NULL)))
	{
		owner_pid = 0;
		update = TRUE;
	}

	// If we are ready to bind (from above, or because we picked arti up from it's load spot)
	if (!owner_pid && IS_PC(ch))
	{
		// Set the artifact to the player
		act("&+L$p &+Lmerges with your &+wsoul&+L.", FALSE, ch, arti, 0, TO_CHAR);
		owner_pid = GET_PID(ch);
	}

	// If object is bound, and not being held by the owner
	if (IS_PC(ch) && (owner_pid != -1) && (owner_pid != GET_PID(ch)))
	{
		// If by some chance, the timer wasn't set set it
		if (!timer)
		{
			logit(LOG_ARTIFACT,
			      "artifact_switch_check: Timer on arti vnum %d was not set.", vnum);
			timer = time(NULL);
			update = TRUE;
		}
		// Otherwise if the timer is due, set it to the new player
		else if ((timer + (60 * (int)get_property("artifact.feeding.switch.timer.min",
							  30))) < time(NULL))
		{
			act("&+L$p &+Lmerges with your &+wsoul&+L.", FALSE, ch, arti, 0, TO_CHAR);
			owner_pid = GET_PID(ch);
			timer = 0;
			update = TRUE;
		}
		// 1% chance to complain.
		else if (!number(0, 99))
		{
			act("&+m$p&+m whimpers softly inside your head.&n", FALSE, ch, arti, 0,
			    TO_CHAR);
		}
	}
	// Or if object is bound by the currently carried player, make sure the timer is reset
	else if (IS_PC(ch) && (owner_pid == GET_PID(ch)))
	{
		timer = 0;
		update = TRUE;
	}

	if (update)
	{
		sql_update_bind_data(vnum, &owner_pid, &timer);
	}
}

// Update the DB with new artifact data.
// arti    : The artifact passed.  Should always be a valid arti, in a valid location.
//             Some error handling is done for invalid locations (mostly move to Limbo).
// owned   : 'Y'/'y' -> set owned in DB, 'N'/'n' -> unset owned in DB, any other value -> leave owned in DB.
// timer   : the new time when the artifact poofs (not the amount of time until it poofs).
// The variable obj1 represents the outer-most object (not in a container) from which we can
//   ascertain the true location of the arti (on a char / in a room / etc).
void artifact_update_sql(P_obj arti, char owned, time_t timer)
{
	int type, locType, location, vnum = arti ? OBJ_VNUM(arti) : -1;
	bool new_owned;
	P_char owner;
	P_obj obj1;
#ifndef __NO_MYSQL__
	bool update_existing = FALSE;
	MYSQL_RES *res;
	MYSQL_ROW row;
#endif

	if (!updateArtis)
	{
		return;
	}

	if (!arti || !IS_ARTIFACT(arti))
	{
		logit(LOG_ARTIFACT, "arti_update_sql: Non arti vnum %d.", vnum);
		return;
	}

	// Figure out the new info.
	type = IS_IOUN(arti) ? ARTIFACT_IOUN : IS_UNIQUE(arti) ? ARTIFACT_UNIQUE : ARTIFACT_MAJOR;

	// Set location and locType here.
	// If we have it in a container, need to get the outer-most one and go from there.
	if (OBJ_INSIDE(arti))
	{
		// Bug handling
		if (!(obj1 = arti->loc.inside))
		{
			logit(LOG_ARTIFACT,
			      "arti_update_sql: OBJ_INSIDE but no container, sending to Limbo, arti vnum %d.",
			      vnum);
			arti->loc_p = LOC_NOWHERE;
			obj_to_room(arti, real_room0(ROOM_LIMBO_VNUM));
		}
		else
		{
			// Get the outer-most container.
			while (OBJ_INSIDE(obj1) && obj1->loc.inside)
			{
				obj1 = obj1->loc.inside;
			}
			// Bug handling.. inside but NULL container.
			if (OBJ_INSIDE(obj1))
			{
				logit(LOG_ARTIFACT,
				      "arti_update_sql: OBJ_INSIDE but container not anywhere, sending to Limbo, arti vnum %d.",
				      vnum);
				obj1->loc_p = LOC_NOWHERE;
				obj_to_room(obj1, real_room0(ROOM_LIMBO_VNUM));
			}
		}
	}
	// If not in a container, just look for where arti is.
	else
	{
		obj1 = arti;
	}

	if (OBJ_WORN(obj1) || OBJ_CARRIED(obj1))
	{
		// More bug handling.
		if (!(owner = (OBJ_WORN(obj1) ? obj1->loc.wearing : obj1->loc.carrying)))
		{
			if (OBJ_WORN(obj1))
			{
				logit(LOG_ARTIFACT,
				      "arti_update_sql: OBJ_WORN but no loc.wearing, arti vnum %d/container vnum %d.",
				      vnum, OBJ_VNUM(obj1));
			}
			else
			{
				logit(LOG_ARTIFACT,
				      "arti_update_sql: OBJ_CARRIED but no loc.carrying, arti vnum %d/container vnum %d.",
				      vnum, OBJ_VNUM(obj1));
			}
			return;
		}
		// Imms can hold unlimited non-tracked artis.
		if (IS_TRUSTED(owner))
		{
			return;
		}
		locType = IS_NPC(owner) ? ARTIFACT_ON_NPC : ARTIFACT_ON_PC;
		// GET_ID returns -2 if owner is not alive.
		location = GET_ID(owner);
	}
	else if (OBJ_ROOM(obj1))
	{
		locType = ARTIFACT_ONGROUND;
		location = obj1->loc.room;
		if (location < 0 || location > top_of_world)
		{
			if (location == NOWHERE)
			{
				logit(LOG_ARTIFACT,
				      "arti_update_sql: OBJ_ROOM but room num NOWHERE, listing as not in game, arti vnum %d.",
				      vnum);
				locType = ARTIFACT_NOTINGAME;
			}
			else
			{
				logit(LOG_ARTIFACT,
				      "arti_update_sql: OBJ_ROOM but room out of bounds, sending to Limbo, arti vnum %d.",
				      vnum);
				obj1->loc_p = LOC_NOWHERE;
				obj_to_room(obj1, real_room0(ROOM_LIMBO_VNUM));
				location = world[obj1->loc.room].number;
			}
		}
		else
		{
			// Convert from valid Rnum to Vnum.
			location = world[location].number;
		}
	}
	// Well, it's sorta a valid location (usually means we're inbetween obj_from_* and obj_to_* fns.
	else if (OBJ_NOWHERE(obj1))
	{
		// If it's on a PC corpse (when corpses are created, they have eq added before being sent to a room,
		//   so they should come up under OBJ_NOWHERE.
		locType = ((obj1->type == ITEM_CORPSE) &&
			   IS_SET(obj1->value[CORPSE_FLAGS], PC_CORPSE)) ?
				  ARTIFACT_ONCORPSE :
				  ARTIFACT_NOTINGAME;
		location = NOWHERE;
	}
	else
	{
		logit(LOG_ARTIFACT,
		      "arti_update_sql: Trying to salvage arti vnum %d (unknown loc_p) to Limbo.",
		      vnum);
		obj1->loc_p = LOC_NOWHERE;
		obj_to_room(obj1, real_room0(ROOM_LIMBO_VNUM));
		locType = ARTIFACT_ONGROUND;
		location = world[obj1->loc.room].number;
	}
	// At this point, type, locType, location, and vnum should be correct.

#ifdef __NO_MYSQL__
	flatfile_artifact_record existing;
	std::string error;
	const auto loaded =
		flatfile_artifact_get(persistence_mode_flatfile_root(), vnum, &existing, &error);
	if (loaded == flatfile_artifact_result::ok)
	{
		new_owned = UPPER(owned) == 'Y' ? TRUE :
			    UPPER(owned) == 'N' ? FALSE :
						  existing.owned;
		if (locType == ARTIFACT_ONCORPSE)
			location = existing.location;
	}
	else if (loaded == flatfile_artifact_result::not_found)
		new_owned = UPPER(owned) == 'Y';
	else
	{
		logit(LOG_ARTIFACT, "arti_update_sql: flat artifact read failed: %s",
		      error.empty() ? "invalid artifact authority" : error.c_str());
		return;
	}
	if (timer <= 0)
	{
		timer = time(NULL) + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
		if (new_owned)
			logit(LOG_ARTIFACT,
			      "arti_update_sql: WARNING: timer was %ld, resetting to 10 days for vnum %d",
			      (long)0, vnum);
	}
	const auto updated = flatfile_artifact_gameplay_update(
		persistence_mode_flatfile_root(), vnum, new_owned, locType, location, timer, type,
		static_cast<int64_t>(time(NULL)), &error);
	if (updated != flatfile_artifact_result::ok &&
	    updated != flatfile_artifact_result::unchanged)
	{
		logit(LOG_ARTIFACT, "arti_update_sql: flat artifact update failed: %s",
		      error.empty() ? "invalid or missing artifact authority" : error.c_str());
		return;
	}
	arti_cache_invalidate();
	return;
#else
	// If we can't query the DB, we have a big issue (only values we care about are time difference and owned value).
	if (!qry("SELECT owned, location, UNIX_TIMESTAMP(timer), UNIX_TIMESTAMP(lastUpdate) FROM artifacts WHERE vnum = %d",
		 vnum))
	{
		logit(LOG_ARTIFACT, "arti_update_sql: failed to read from database.");
		return;
	}
	res = mysql_store_result(DB);
	if (!res)
	{
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		return;
	}
	// Since vnum is unique, num rows should be 0 or 1.
	if (mysql_num_rows(res) < 1 || (row = mysql_fetch_row(res)) == NULL)
	{
		// Only set it to owned if we know that it's owned.
		if (UPPER(owned) == 'Y')
		{
			new_owned = TRUE;
		}
		else
		{
			new_owned = FALSE;
		}
	}
	else
	{
		if (UPPER(owned) == 'Y')
		{
			new_owned = TRUE;
		}
		else if (UPPER(owned) == 'N')
		{
			new_owned = FALSE;
		}
		else
		{
			new_owned = (!strcmp(row[0], "Y")) ? TRUE : FALSE;
		}

		// If it's on a corpse, it should be on the corpse of the last owner,
		//   so we don't want to move it to NOWHERE.
		if (locType == ARTIFACT_ONCORPSE)
		{
			location = atoi(row[1]);
		}

		update_existing = TRUE;
	}

	mysql_free_result(res);

	// If we have an entry already in the DB, update it.
	if (update_existing)
	{
		// Arih : Validate timer to prevent MySQL error "Incorrect datetime value: '1970-01-01 00:00:00'".
		// FROM_UNIXTIME(0) causes MySQL to reject the datetime.
		if (timer <= 0)
		{
			timer = time(NULL) +
				ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY; // 10 days, not 60 secs
			// An unowned artifact has no ticking timer, so 0 is its normal
			// state; only an owned one with no timer is worth flagging.
			if (new_owned)
				logit(LOG_ARTIFACT,
				      "arti_update_sql (UPDATE): WARNING: timer was %ld, resetting to 10 days for vnum %d",
				      (long)0, vnum);
		}

		qry("UPDATE artifacts SET owned='%c', locType=%d, location=%d, timer=FROM_UNIXTIME(%lu), type=%d, lastUpdate=SYSDATE() WHERE vnum=%d",
		    new_owned ? 'Y' : 'N', locType, location, timer, type, vnum);
		arti_cache_invalidate();
	}
	// Otherwise, create one.
	else
	{
		logit(LOG_ARTIFACT,
		      "arti_update_sql: Creating entry: vnum: %d, new_owned: %c, locType: %d, location; %d, timer: %lu, type: %d.",
		      vnum, new_owned ? 'Y' : 'N', locType, location, timer, type);

		// Arih : Validate timer to prevent MySQL error "Incorrect datetime value: '1970-01-01 00:00:00'".
		// FROM_UNIXTIME(0) causes MySQL to reject the datetime.
		if (timer <= 0)
		{
			timer = time(NULL) +
				ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY; // 10 days, not 60 secs
			if (new_owned)
				logit(LOG_ARTIFACT,
				      "arti_update_sql: WARNING: timer was %ld, resetting to 10 days for vnum %d",
				      (long)0, vnum);
		}

		qry("INSERT INTO artifacts (vnum, owned, locType, location, timer, type, lastUpdate) VALUES( %d, '%c', %d, %d, FROM_UNIXTIME(%lu), %d, SYSDATE())",
		    vnum, new_owned ? 'Y' : 'N', locType, location, timer, type);
		arti_cache_invalidate();
	}
#endif
}

// This function just updates/creates a new entry for the arti with vnum vnum.
void artifact_update_sql(int vnum, bool owned, int locType, int location, time_t timer, int type)
{
#ifndef __NO_MYSQL__
	bool update_existing;
	MYSQL_RES *res;
	MYSQL_ROW row;
#endif

	if (!updateArtis)
	{
		return;
	}

#ifdef __NO_MYSQL__
	if (timer <= 0)
	{
		timer = time(NULL) + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
		if (owned)
			logit(LOG_ARTIFACT,
			      "artifact_update_sql: WARNING: timer was %ld, resetting to 10 days for vnum %d",
			      (long)0, vnum);
	}
	std::string error;
	const auto updated = flatfile_artifact_gameplay_update(
		persistence_mode_flatfile_root(), vnum, owned, locType, location, timer, type,
		static_cast<int64_t>(time(NULL)), &error);
	if (updated != flatfile_artifact_result::ok &&
	    updated != flatfile_artifact_result::unchanged)
	{
		logit(LOG_ARTIFACT, "artifact_update_sql: flat artifact update failed: %s",
		      error.empty() ? "invalid or missing artifact authority" : error.c_str());
		return;
	}
	arti_cache_invalidate();
	return;
#else
	// If we can't query the DB, we have a big issue (only values we care about are time difference and owned value).
	if (!qry("SELECT owned, location, UNIX_TIMESTAMP(timer), UNIX_TIMESTAMP(lastUpdate) FROM artifacts WHERE vnum = %d",
		 vnum))
	{
		logit(LOG_ARTIFACT, "arti_update_sql: failed to read from database.");
		return;
	}
	res = mysql_store_result(DB);
	if (!res)
	{
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		return;
	}

	// Since vnum is unique, num rows should be 0 or 1.
	if (mysql_num_rows(res) < 1 || (row = mysql_fetch_row(res)) == NULL)
	{
		update_existing = FALSE;
	}
	else
	{
		update_existing = TRUE;
	}
	mysql_free_result(res);

	// Arih : Validate timer to prevent MySQL error "Incorrect datetime value: '1970-01-01 00:00:00'".
	// FROM_UNIXTIME(0) causes MySQL to reject the datetime.
	if (timer <= 0)
	{
		timer = time(NULL) +
			ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY; // 10 days, not 60 secs
		// Only an owned artifact has a ticking timer, so 0 is normal otherwise.
		if (owned)
			logit(LOG_ARTIFACT,
			      "artifact_update_sql: WARNING: timer was %ld, resetting to 10 days for vnum %d",
			      (long)0, vnum);
	}

	if (update_existing)
	{
		qry("UPDATE artifacts SET owned='%c', locType=%d, location=%d, timer=FROM_UNIXTIME(%lu), type=%d, lastUpdate=SYSDATE() WHERE vnum=%d",
		    owned ? 'Y' : 'N', locType, location, timer, type, vnum);
		arti_cache_invalidate();
	}
	else
	{
		qry("INSERT INTO artifacts (vnum, owned, locType, location, timer, type, lastUpdate) VALUES(%d, '%c', %d, %d, FROM_UNIXTIME(%lu), %d, SYSDATE())",
		    vnum, owned ? 'Y' : 'N', locType, location, timer, type);
		arti_cache_invalidate();
	}
#endif
}

// Remove the artifact data from the DB.
//   arti : The artifact to remove.
//   pid  : Whether it goes to the corpse of char who's PID is pid, or <= 0 means arti is gone from game.
// Returns true if successfully removed.
bool remove_owned_artifact_sql(P_obj arti, int pid)
{
	int vnum = arti ? OBJ_VNUM(arti) : -1;
#ifndef __NO_MYSQL__
	bool update_existing = FALSE;
	MYSQL_RES *res;
	MYSQL_ROW row = NULL;
#endif

	if (!updateArtis)
	{
		return FALSE;
	}

	// Bad arguments...
	if (!arti || !IS_ARTIFACT(arti))
	{
		logit(LOG_ARTIFACT, "remove_owned_artifact_sql: called with non-artifact '%s' %d.",
		      arti ? arti->short_description : "NULL", arti ? OBJ_VNUM(arti) : -1);
		return FALSE;
	}

#ifdef __NO_MYSQL__
	const int type = IS_IOUN(arti)	 ? ARTIFACT_IOUN :
			 IS_UNIQUE(arti) ? ARTIFACT_UNIQUE :
					   ARTIFACT_MAJOR;
	std::string error;
	const auto removed = flatfile_artifact_remove_owned(persistence_mode_flatfile_root(), vnum,
							    pid, type, time(NULL), &error);
	if (removed != flatfile_artifact_result::ok &&
	    removed != flatfile_artifact_result::unchanged)
	{
		logit(LOG_ARTIFACT, "remove_owned_artifact_sql: flat artifact update failed: %s",
		      error.empty() ? "invalid or missing artifact authority" : error.c_str());
		return FALSE;
	}
	arti_cache_invalidate();
	return TRUE;
#else

	// If we can't query the DB, we have a big issue (only values we care about are time difference and owned value).
	if (!qry("SELECT owned, UNIX_TIMESTAMP(timer), UNIX_TIMESTAMP(lastUpdate) FROM artifacts WHERE vnum = %d",
		 vnum))
	{
		logit(LOG_ARTIFACT, "remove_owned_artifact_sql: failed to read from database.");
		return FALSE;
	}
	res = mysql_store_result(DB);
	if (!res)
	{
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		return FALSE;
	}
	if (mysql_num_rows(res) > 0 && !(row = mysql_fetch_row(res)))
	{
		logit(LOG_ARTIFACT, "remove_owned_artifact_sql: failed to fetch row?!");
		mysql_free_result(res);
		return FALSE;
	}
	if (row)
	{
		update_existing = TRUE;
	}
	mysql_free_result(res);

	// If there's an existing entry in the DB.
	if (update_existing)
	{
		// Non-positive pid -> remove arti from game.
		if (pid <= 0)
		{
			qry("UPDATE artifacts SET owned='N', locType=%d, location=%d, lastUpdate=SYSDATE() WHERE vnum=%d",
			    ARTIFACT_NOTINGAME, NOWHERE, vnum);
			arti_cache_invalidate();
		}
		// Otherwise, we're moving to a corpse of char who's PID is pid.
		else
		{
			// On a PC corpse -> owned == Yes, and location == pid.
			qry("UPDATE artifacts SET owned='Y', locType=%d, location=%d, lastUpdate=SYSDATE() WHERE vnum=%d",
			    ARTIFACT_ONCORPSE, pid, vnum);
			arti_cache_invalidate();
		}
	}
	// If the entry doesn't exist and we're moving arti to a corpse (Yes, this would be a buggy situation).
	else if (pid > 0)
	{
		// On a PC corpse -> owned == 'Y', locType == 'OnCorpse', and location == pid.
		qry("INSERT INTO artifacts (vnum, owned, locType, location, timer, type, lastUpdate) VALUES(%d, 'Y', %d, %d, 0, %d, SYSDATE())",
		    vnum, ARTIFACT_ONCORPSE, pid,
		    IS_IOUN(arti)   ? ARTIFACT_IOUN :
		    IS_UNIQUE(arti) ? ARTIFACT_UNIQUE :
				      ARTIFACT_MAJOR);
		arti_cache_invalidate();
	}

	// Safe to assume that a poofed arti has an entry in artifact_bind.  We don't really care either way though,
	//   as long as it doesn't have an entry with a pid after poof.
	qry("UPDATE artifact_bind SET owner_pid = -1, timer = 0 WHERE vnum = %d", vnum);

	// If pid <= 0 && there's no existing entry, don't bother.
	return TRUE;
#endif
}

// This is used for when a character is deleted.
void remove_all_artifacts_sql(P_char ch)
{
	int pid;

	if (!updateArtis)
	{
		return;
	}

	// If no ch / ch isn't a PC / or ch doesn't have PC data.
	if (!ch || !IS_PC(ch) || !ch->only.pc)
	{
		return;
	}
	pid = GET_PID(ch);

#ifdef __NO_MYSQL__
	std::string error;
	const auto removed =
		flatfile_artifact_release_player(persistence_mode_flatfile_root(), pid, &error);
	if (removed != flatfile_artifact_result::ok &&
	    removed != flatfile_artifact_result::unchanged)
	{
		logit(LOG_ARTIFACT, "remove_all_artifacts_sql: flat artifact release failed: %s",
		      error.empty() ? "missing or invalid artifact authority" : error.c_str());
		return;
	}
#else
	// Nullify arti timers on all ch's equipment.
	qry("UPDATE artifacts SET owned='N', timer=NULL, lastUpdate=SYSDATE() WHERE location=%d and locType=%d",
	    pid, ARTIFACT_ON_PC);
#endif
	arti_cache_invalidate();
}

// This is a wrapper function for artifact_update_sql.
// It merely calculates the timer and location, then calls the above with appropriate owned/timer info.
void artifact_update_location_sql(P_obj arti)
{
	bool timerStarted;
	arti_data artidata;
	P_obj cont = arti;
	P_char owner;

	if (!arti || !IS_ARTIFACT(arti) || !updateArtis)
	{
		return;
	}

	// Get data from DB.
	timerStarted = get_artifact_data_sql(OBJ_VNUM(arti), &artidata);

	// Get outer-most container.
	while (OBJ_INSIDE(cont) && cont->loc.inside)
	{
		cont = cont->loc.inside;
	}

	if (OBJ_WORN(cont) || OBJ_CARRIED(cont))
	{
		owner = OBJ_WORN(cont) ? cont->loc.wearing : cont->loc.carrying;
		if (IS_NPC(owner))
		{
			// NPCs don't start the timer.
			artifact_update_sql(arti, '0', timerStarted ? artidata.timer : 0);
		}
		// PCs get a new timer if it wasn't previously owned.
		else
		{
			artifact_switch_check(owner, arti);
			if (!timerStarted)
			{
				// Set the timer to the max for a newly acquired arti.
				artidata.timer =
					time(NULL) + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
			}
			// PC owner forces a start to timer.
			artifact_update_sql(arti, 'Y', artidata.timer);
		}
	}
	// It's in a room / container / corpse / nowhere: We don't change the timer.
	else
	{
		artifact_update_sql(arti, '0', timerStarted ? artidata.timer : 0);
	}
}

// Returns TRUE iff arti vnum has timer ticking already.
bool get_artifact_data_sql(int vnum, P_arti adata)
{
#ifdef __NO_MYSQL__
	flatfile_artifact_record record;
	std::string error;
	const auto loaded =
		flatfile_artifact_get(persistence_mode_flatfile_root(), vnum, &record, &error);
	if (loaded == flatfile_artifact_result::not_found)
		return FALSE;
	if (loaded != flatfile_artifact_result::ok)
	{
		logit(LOG_ARTIFACT, "get_artifact_data_sql: flat artifact read failed: %s",
		      error.empty() ? "invalid artifact authority" : error.c_str());
		return FALSE;
	}
	if (adata)
	{
		adata->vnum = record.vnum;
		adata->owned = record.owned;
		adata->locType = static_cast<char>(record.location_type);
		adata->location = record.location;
		adata->timer = static_cast<time_t>(record.timer);
		adata->type = static_cast<char>(record.type);
		adata->next = NULL;
	}
	return record.owned;
#else
	bool owned;
	MYSQL_RES *res;
	MYSQL_ROW row = NULL;

	if (!qry("SELECT owned, locType, location, UNIX_TIMESTAMP(timer), type FROM artifacts WHERE vnum = %d",
		 vnum))
	{
		logit(LOG_ARTIFACT, "get_artifact_data_sql: failed to read from database.");
		return FALSE;
	}
	res = mysql_store_result(DB);
	if (!res)
	{
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		return FALSE;
	}

	// Non-buggy no row for arti.
	if (mysql_num_rows(res) <= 0)
	{
		mysql_free_result(res);
		return FALSE;
	}

	if (!(row = mysql_fetch_row(res)))
	{
		logit(LOG_ARTIFACT, "get_artifact_data_sql: failed to fetch row?!");
		mysql_free_result(res);
		return FALSE;
	}
	owned = (!strcmp(row[0], "Y")) ? TRUE : FALSE;
	if (adata != NULL)
	{
		adata->vnum = vnum;
		adata->owned = owned;
		adata->locType = atoi(row[1]);
		adata->location = atoi(row[2]);
		adata->timer = row[3] ? atol(row[3]) : 0;
		adata->type = atoi(row[4]);
		adata->next = NULL;
	}
	mysql_free_result(res);
	return owned;
#endif
}

void artifact_feed_sql(P_char owner, P_obj arti, int feed_seconds, bool soulCheck)
{
	int vnum, owner_pid, timer;
	arti_data artidata;
	time_t poof_time;
	bool negFeed = FALSE;

	if (!updateArtis)
	{
		return;
	}

	if (!owner || !arti || IS_NPC(owner))
	{
		statuslog(MINLVLIMMORTAL,
			  "artifact_feed_sql: called with null / NPC owner or NULL arti.");
		debug("artifact_feed_sql: called with owner (%s) and arti (%s) %d.",
		      owner ? J_NAME(owner) : "NULL", arti ? arti->short_description : "NULL",
		      arti ? OBJ_VNUM(arti) : -1);
		return;
	}
	if (IS_TRUSTED(owner))
	{
		return;
	}

	vnum = OBJ_VNUM(arti);
	if (!sql_get_bind_data(vnum, &owner_pid, &timer))
	{
		return;
	}

	// Anti artifact sharing for feeding check
	if (soulCheck && IS_PC(owner) && (owner_pid != -1) && (owner_pid != GET_PID(owner)))
	{
		act("&+L$p &+Lhas yet to accept you as its owner.", FALSE, owner, arti, 0, TO_CHAR);
		return;
	}

	// Get data from DB.
	poof_time = time(NULL) + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;

	// Arih : Validate poof_time to prevent MySQL error "Incorrect datetime value: '1970-01-01 00:00:00'".
	// FROM_UNIXTIME(0) causes MySQL to reject the datetime.
	if (poof_time <= 0)
	{
		poof_time = time(NULL) +
			    ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY; // 10 days, not 60 secs
		logit(LOG_ARTIFACT,
		      "artifact_feed_sql: WARNING: poof_time was %ld, resetting to 10 days for vnum %d",
		      (long)0, vnum);
	}

	if (!get_artifact_data_sql(vnum, &artidata))
	{
		statuslog(MINLVLIMMORTAL, "artifact_feed_sql: called without an entry in DB?!");
#ifdef __NO_MYSQL__
		std::string error;
		const int type = IS_IOUN(arti)	 ? ARTIFACT_IOUN :
				 IS_UNIQUE(arti) ? ARTIFACT_UNIQUE :
						   ARTIFACT_MAJOR;
		const auto updated = flatfile_artifact_gameplay_update(
			persistence_mode_flatfile_root(), vnum, true, ARTIFACT_ON_PC,
			GET_PID(owner), poof_time, type, time(NULL), &error);
		if (updated != flatfile_artifact_result::ok &&
		    updated != flatfile_artifact_result::unchanged)
		{
			logit(LOG_ARTIFACT, "artifact_feed_sql: flat artifact update failed: %s",
			      error.empty() ? "invalid or missing artifact authority" :
					      error.c_str());
			return;
		}
#endif
		send_to_char("&+RYou feel a deep sense of satisfaction from somewhere...\r\n",
			     owner);
#ifndef __NO_MYSQL__
		qry("INSERT INTO artifacts (vnum, owned, locType, location, timer, type, lastUpdate) VALUES(%d, 'Y', %d, %d, FROM_UNIXTIME(%lu), %d, SYSDATE())",
		    vnum, ARTIFACT_ON_PC, GET_PID(owner), poof_time,
		    IS_IOUN(arti) ? ARTIFACT_IOUN :
				    (IS_UNIQUE(arti) ? ARTIFACT_UNIQUE : ARTIFACT_MAJOR));
#endif
		arti_cache_invalidate();
		return;
	}

	// If we're tyring to feed over the limit.
	if (poof_time < artidata.timer + feed_seconds)
	{
		// Set it to feed to max
		feed_seconds = poof_time - artidata.timer;
	}

	if (feed_seconds > (12 * 3600))
	{
		send_to_char("&+RYou feel a deep sense of satisfaction from somewhere...&n\r\n",
			     owner);
	}
	else if (feed_seconds < 0)
	{
		negFeed = TRUE;
		feed_seconds *= -1;
		send_to_char("&+BYou hear a distant cry in the back of your mind...&n\n\r", owner);
	}
	else
	{
		send_to_char("&+RYou feel a light sense of satisfaction from somewhere...&n\r\n",
			     owner);
	}

	statuslog(MINLVLIMMORTAL, "Artifact: %s [%d] on %s fed [&+G%c%d&+Lh &+G%d&+Lm &+G%d&+Ls&n]",
		  arti->short_description, OBJ_VNUM(arti), GET_NAME(owner), negFeed ? '-' : ' ',
		  feed_seconds / 3600, (feed_seconds / 60) % 60, feed_seconds % 60);

	if (artidata.owned &&
	    (artidata.locType == ARTIFACT_ON_PC || artidata.locType == ARTIFACT_ONCORPSE) &&
	    artidata.location != GET_PID(owner))
	{
		statuslog(
			MINLVLIMMORTAL,
			"artifact_feed_sql: tried to track arti vnum %d on %s when already tracked on %s.",
			vnum, GET_NAME(owner), get_player_name_from_pid(artidata.location));
		return;
	}
	artifact_update_sql(arti, 'Y', artidata.timer + feed_seconds);
}

// Loads up a dummy copy of char 'name'.
P_char load_dummy_char(char *name)
{
	P_char owner;

	// Get the memory
	owner = (P_char)mm_get(dead_mob_pool);
	clear_char(owner);
	ensure_pconly_pool();
	owner->only.pc = (struct pc_only_data *)mm_get(dead_pconly_pool);
	owner->desc = NULL;

	if (restoreCharOnly(owner, name) < 0)
	{
		logit(LOG_ARTIFACT, "load_dummy_char: %s has bad / missing pfile.\n\r", name);
		free_char(owner);
		return NULL;
	}

	updateArtis = FALSE;
	restoreItemsOnly(owner, -1);
	owner->next = character_list;
	character_list = owner;
	updateArtis = TRUE;

	return owner;
}

// Return a pointer to the first obj of vnum vnum on owner.
//   Checks eq'd eq first.
//   Does not check inside containers.
// Does not remove the object (this would ruin poof_artifact)!
P_obj get_object_from_char(P_char owner, int vnum)
{
	int i;
	P_obj obj;

	// Check eq'd stuff first.
	for (i = 0; i < MAX_WEAR; i++)
	{
		if (!owner->equipment[i])
			continue;
		if (obj_index[owner->equipment[i]->R_num].virtual_number == vnum)
			return owner->equipment[i];
	}
	// Then check inventory.
	for (obj = owner->carrying; obj; obj = obj->next_content)
	{
		if (obj_index[obj->R_num].virtual_number == vnum)
			return obj;
	}
	// If they don't have it.
	return NULL;
}

void poof_artifact(P_obj arti)
{
	P_char owner;
	P_obj cont;

	if (!updateArtis)
	{
		return;
	}

	if (!IS_ARTIFACT(arti))
	{
		logit(LOG_ARTIFACT, "poof_artifact: Non-arti vnum %d!", OBJ_VNUM(arti));
		return;
	}

	cont = arti;
	while (OBJ_INSIDE(cont) && cont->loc.inside)
	{
		cont = cont->loc.inside;
	}

	owner = NULL;
	// Pull artifact from wherever it is.
	switch (cont->loc_p)
	{
	case LOC_ROOM:
		if (cont != arti)
		{
			obj_from_obj(arti);
			obj_to_room(arti, cont->loc.room);
		}
		act("$p suddenly vanishes in a bright flash of light!", FALSE, NULL, arti, 0,
		    TO_ROOM);
		break;
	case LOC_CARRIED:
		owner = cont->loc.carrying;
		if (cont != arti)
		{
			obj_from_obj(arti);
			obj_to_char(arti, owner);
		}
		act("Your $p vanishes with a bright flash of light!", FALSE, owner, arti, 0,
		    TO_CHAR);
		act("$n's $p suddenly vanishes in a bright flash of light!", FALSE, owner, arti, 0,
		    TO_ROOM);
		do_shout(owner, writable_arg("Ouch!"), 0);
		break;
	case LOC_WORN:
		owner = cont->loc.wearing;
		if (cont != arti)
		{
			obj_from_obj(arti);
			obj_to_char(arti, owner);
		}
		act("Your $p vanishes with a bright flash of light!", FALSE, owner, arti, 0,
		    TO_CHAR);
		act("$n's $p suddenly vanishes in a bright flash of light!", FALSE, owner, arti, 0,
		    TO_ROOM);
		do_shout(owner, writable_arg("Ouch!"), 0);
		break;
	case LOC_INSIDE:
		logit(LOG_ARTIFACT, "poof_artifact: Bad loc arti(%d)-container(%d) inside nothing.",
		      OBJ_VNUM(arti), OBJ_VNUM(cont));
	case LOC_NOWHERE:
	default:
		break;
	}
	if (cont == arti)
	{
		cont = NULL;
	}

	// Logit.
	logit(LOG_ARTIFACT, "poof_artifact: Poofing '%s' %d from %s!", OBJ_SHORT(arti),
	      OBJ_VNUM(arti),
	      owner	     ? J_NAME(owner) :
	      OBJ_ROOM(arti) ? world[arti->loc.room].name :
			       "unknown location");

	// And get rid of it.
	extract_obj(arti, TRUE);
	// Save where appropriate (We can only save if owner is online active, since we don't know if owner is
	//   offline and a dummy, where we'd write RENT_POOFARTI, or if the owner is just ld, where we'd write
	//   RENT_CRASH).  However, it's harmless to save them RENT_CRASH, so we'll do that regardless.
	if (owner)
	{
		writeCharacter(owner, RENT_CRASH, owner->in_room);
	}
	if (cont != NULL && OBJ_VNUM(cont) == VOBJ_CORPSE &&
	    IS_SET(cont->value[CORPSE_FLAGS], PC_CORPSE))
	{
		writeCorpse(cont);
	}
}

// Removes an unsaved char's equipment from game without disturbing arti list.
void nuke_eq(P_char ch)
{
	int i;
	P_obj item;

	for (i = 0; i < MAX_WEAR; i++)
	{
		if (ch->equipment[i])
		{
			item = unequip_char(ch, i);
			// This must be FALSE to prevent nuking arti files.
			extract_obj(item, FALSE);
		}
	}
	while (ch->carrying)
	{
		item = ch->carrying;
		obj_from_char(item);
		// This must be FALSE to prevent nuking arti files.
		extract_obj(item, FALSE);
	}
}

// Takes the arti vnum, and turns it into a text string (in buffer) displaying
//   the time left on the corresponding artifact.
void artifact_timer_sql(int vnum, char *buffer)
{
	arti_data artidata;
	time_t timer;

	if (!updateArtis)
	{
		return;
	}

	if (get_artifact_data_sql(vnum, &artidata))
	{
		if (artidata.owned)
		{
			timer = artidata.timer - time(NULL);
		}
		else
		{
			snprintf(buffer, MAX_STRING_LENGTH, "[ &=LRUnknown&n ]");
			return;
		}
		if (timer < 0)
		{
			timer *= -1;
			snprintf(buffer, MAX_STRING_LENGTH, "[&+R-%ld&+Lh &+R%ld&+Lm &+R%ld&+Ls&n]",
				 timer / 3600, (timer / 60) % 60, timer % 60);
		}
		else if (timer <= (ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY) / 16)
		{
			snprintf(buffer, MAX_STRING_LENGTH, "[&+R%ld&+Lh &+R%ld&+Lm &+R%ld&+Ls&n]",
				 timer / 3600, (timer / 60) % 60, timer % 60);
		}
		else if (timer <= (ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY) / 5)
		{
			snprintf(buffer, MAX_STRING_LENGTH, "[&+Y%ld&+Lh &+Y%ld&+Lm &+Y%ld&+Ls&n]",
				 timer / 3600, (timer / 60) % 60, timer % 60);
		}
		else
		{
			snprintf(buffer, MAX_STRING_LENGTH, "[&+G%ld&+Lh &+G%ld&+Lm &+G%ld&+Ls&n]",
				 timer / 3600, (timer / 60) % 60, timer % 60);
		}
	}
	else
	{
		snprintf(buffer, MAX_STRING_LENGTH, "[ &=LCUnknown&n ]");
	}
}

// Recursive function to search through bags for the first instance of obj vnum.
//   First does not necessarily mean the outermost instance (although there should only be one.)
P_obj artifact_in_bag_search(P_obj contents, int vnum)
{
	P_obj temp, inside;

	// Look through the contents.
	for (temp = contents; temp; temp = temp->next_content)
	{
		// Found it.
		if (OBJ_VNUM(temp) == vnum)
		{
			return temp;
		}
		// If we have a container in a container, search those contents.
		if (temp->contains)
		{
			// If it's inside those contents, return it.
			if ((inside = artifact_in_bag_search(temp->contains, vnum)))
			{
				return inside;
			}
		}
	}
	// Didn't find it. :(
	return NULL;
}

// Finds an artifact in game not on an Immortal or PC or PC corpse.
// The goal of this function is to find the artifact where it loads in game before it's acquired by a PC.
P_obj artifact_find(int vnum)
{
	P_char owner;
	P_obj obj, cont;

	for (obj = object_list; obj; obj = obj->next)
	{
		if (OBJ_VNUM(obj) == vnum)
		{
			cont = obj;
			while (OBJ_INSIDE(cont) && cont->loc.inside)
			{
				cont = cont->loc.inside;
			}
			switch (cont->loc_p)
			{
			// Score!
			case LOC_ROOM:
				return obj;
				break;
			case LOC_CARRIED:
			case LOC_WORN:
				owner = OBJ_WORN(cont) ? cont->loc.wearing : cont->loc.carrying;
				if (IS_NPC(owner))
				{
					return obj;
				}
				break;
			case LOC_INSIDE:
				logit(LOG_ARTIFACT,
				      "artifact_find: Bad loc arti(%d)-container(%d) inside nothing.",
				      vnum, OBJ_VNUM(cont));
				break;
			// Lost artis. :(
			case LOC_NOWHERE:
			default:
				break;
			}
		}
	}

	return NULL;
}

// This function finds the artifact corresponding to artidata.
// It does not return an artifact that isn't on the proper PC corpse for example.
// It also just searches in game.  If you want to search pfiles.  Do it after.
P_obj artifact_find(arti_data artidata)
{
	int rroom, pid;
	int vnum = artidata.vnum;
	int i;
	char *name;
	P_obj obj, inside;
	P_char ch;

	switch (artidata.locType)
	{
	case ARTIFACT_ON_NPC:
		pid = artidata.location;
		for (ch = character_list; ch; ch = ch->next)
		{
			if (IS_NPC(ch) && GET_VNUM(ch) == pid)
			{
				// Check eq worn first, since they will probably be wearing it.
				for (i = 0; i < MAX_WEAR; i++)
				{
					if ((obj = ch->equipment[i]))
					{
						if (OBJ_VNUM(obj) == vnum)
						{
							return obj;
						}
						// If they're wearing a container.
						if (obj->contains &&
						    (inside = artifact_in_bag_search(obj->contains,
										     vnum)))
						{
							return inside;
						}
					}
				}
				// If they're not wearing it, then check inventory.  We do have to keep looking since
				//   there can be multiple NPCs with the same vnum.
				if ((inside = artifact_in_bag_search(ch->carrying, vnum)))
				{
					return inside;
				}
			}
		}
		break;
	// Since they may be LD, we have to check all chars, not just descriptors. :(
	case ARTIFACT_ON_PC:
		pid = artidata.location;
		for (ch = character_list; ch; ch = ch->next)
		{
			// Yay, we found them in game!
			if (IS_PC(ch) && GET_PID(ch) == pid)
			{
				// Check eq worn first, since they will probably be wearing it.
				for (i = 0; i < MAX_WEAR; i++)
				{
					if ((obj = ch->equipment[i]))
					{
						if (OBJ_VNUM(obj) == vnum)
						{
							return obj;
						}
						// If they're wearing a container.
						if (obj->contains &&
						    (inside = artifact_in_bag_search(obj->contains,
										     vnum)))
						{
							return inside;
						}
					}
				}
				// If they're not wearing it, then check inventory.  We don't have to keep looking since
				//   each PC is unique in game.
				return artifact_in_bag_search(ch->carrying, vnum);
			}
		}
		// Here it gets hard: do we check the pfile or not?  I say no, since we don't have a way
		//   to dispose of the loaded char after the arti is handled.  If you want to check a pfile,
		//   you'll just have to do it after this function returns NULL.
		return NULL;
		break;
	// Since we don't have the room the corpse is in, we have to go through the whole object list.
	case ARTIFACT_ONCORPSE:
		name = get_player_name_from_pid(artidata.location);
		for (obj = object_list; obj; obj = obj->next)
		{
			// If we found a PC corpse with the correct name.  I would so like to start using pids
			//   instead of names for corpses.
			if (obj->type == ITEM_CORPSE &&
			    IS_SET(obj->value[CORPSE_FLAGS], PC_CORPSE) && isname(name, obj->name))
			{
				if ((inside = artifact_in_bag_search(obj->contains, vnum)))
				{
					return inside;
				}
			}
		}
		return NULL;
		break;
	case ARTIFACT_ONGROUND:
		// Bad room vnum on ground data.
		if ((rroom = real_room(artidata.location)) < 0 || rroom > top_of_world)
		{
			return NULL;
		}
		// Search the room's contents.
		for (obj = world[rroom].contents; obj; obj = obj->next_content)
		{
			if (OBJ_VNUM(obj) == vnum)
			{
				return obj;
			}
			// If we've found a container.
			if (obj->contains)
			{
				// Look for the object inside the container.
				if ((inside = artifact_in_bag_search(obj->contains, vnum)))
				{
					return inside;
				}
			}
		}
		return NULL;
	case ARTIFACT_NOTINGAME:
	default:
		return NULL;
		break;
	}
	// Should never get here, but just in case.
	return NULL;
}

// This function transfers the data from the old file-based system into the DB.
void arti_files_to_sql(P_char ch, char *arg)
{
	char buf[MAX_STRING_LENGTH];
	char pname[256], fname[256];
	int vnum, pid, temp, type;
	time_t timer;
	DIR *dir;
	struct dirent *dire;
	FILE *f;
	P_obj arti, obj, obj2;
	P_char tmpch, owner;
	arti_data artidata;
	bool super;

	if (!*arg || !strcmp(arg, "?") || !strcmp(arg, "help"))
	{
		send_to_char(
			"This command transfers the data from the old file-based system into the DB.\n\r",
			ch);
		send_to_char("Please note: This command may pull artifacts from mobs in game.\n\r",
			     ch);
		send_to_char("&=LRThis command should only be used once!&n\n\r", ch);
		return;
	}
	if (GET_LEVEL(ch) < OVERLORD)
	{
		send_to_char("This command is reserved for use by &+rOverlords&n only.\n\r", ch);
		return;
	}
	if (strcmp(arg, "confirm") && strcmp(arg, "super"))
	{
		send_to_char(
			"This command is requires &+wconfirmation&n.  Please do not use if you don't know"
			" what you're doing, as it can corrupt the current artifact data.\n\r",
			ch);
		return;
	}
	if (!strcmp(arg, "super"))
	{
		super = TRUE;
	}
	else
	{
		super = FALSE;
	}

	// At this point, it's a go.
	dir = opendir(ARTIFACT_DIR);

	if (!dir)
	{
		snprintf(buf, MAX_STRING_LENGTH, "Could not open arti dir (%s)\r\n", ARTIFACT_DIR);
		send_to_char(buf, ch);
		return;
	}

	while ((dire = readdir(dir)))
	{
		vnum = atoi(dire->d_name);
		if (!vnum)
		{
			continue;
		}
		snprintf(fname, 256, ARTIFACT_DIR "%d", vnum);
		snprintf(buf, MAX_STRING_LENGTH, "Loading artifact file '%s'.\n\r", fname);
		send_to_char(buf, ch);

		if (!(f = fopen(fname, "rt")))
		{
			logit(LOG_ARTIFACT, "arti_files_to_sql: Could not open file '%s'.", fname);
			continue;
		}

		// Init name to empty string.
		pname[0] = '\0';
		/* Present in the legacy file format and consumed by the fscanf
		   below; the migration stamps lastUpdate with SYSDATE() instead. */
		[[maybe_unused]] long last_update_value;
		long timer_value;
		if (fscanf(f, "%s %d %ld %d %ld\n", pname, &pid, &last_update_value, &temp,
			   &timer_value) != 5)
		{
			logit(LOG_ARTIFACT, "arti_files_to_sql: Could not read file '%s'.", fname);
			fclose(f);
			continue;
		}
		timer = static_cast<time_t>(timer_value);
		fclose(f);

		timer += ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
		snprintf(buf, MAX_STRING_LENGTH, "  Char '%s' %d, has arti %d (%s) with timer %s",
			 pname, pid, vnum,
			 (temp == 0) ? "on char" : ((temp == 1) ? "on corpse" : "unknown location"),
			 ctime(&timer));
		send_to_char(buf, ch);

		if (get_artifact_data_sql(vnum, &artidata))
		{
			// If it's owned by some other PC.
			if (artidata.owned && (artidata.locType == ARTIFACT_ON_PC ||
					       artidata.locType == ARTIFACT_ONCORPSE))
			{
				snprintf(
					buf, MAX_STRING_LENGTH,
					"  &+rConflicts with data already in DB: Char '%s' %d (%s).. skipping...&n\n\r",
					get_player_name_from_pid(artidata.location),
					artidata.location,
					(artidata.locType == ARTIFACT_ON_PC) ?
						"on char" :
						((artidata.locType == ARTIFACT_ONCORPSE) ?
							 "on corpse" :
							 "unknown location"));
				send_to_char(buf, ch);
				continue;
			}
			// Otherwise, pull it from ground/NPC/etc.
			else if ((arti = artifact_find(artidata)))
			{
				snprintf(
					buf, MAX_STRING_LENGTH,
					"Found another copy of arti %s (%d) in game, pulling it.\n\r",
					OBJ_SHORT(arti), OBJ_VNUM(arti));
				send_to_char(buf, ch);
				extract_obj(arti, FALSE);
			}
		}

		if ((arti = read_object(vnum, VIRTUAL)))
		{
			type = IS_IOUN(arti)   ? ARTIFACT_IOUN :
			       IS_UNIQUE(arti) ? ARTIFACT_UNIQUE :
						 ARTIFACT_MAJOR;

			// If there's another in the game (maybe on a mob / treasure room / chest / etc).
			if (obj_index[arti->R_num].number > 1)
			{
				// Look for all other copies (this may piss off Imms with artis in bags or such).
				for (obj = object_list; obj; obj = obj2)
				{
					obj2 = obj->next;

					// Remove it if we find it not on a PC.
					if (obj->R_num == arti->R_num && obj != arti)
					{
						if (OBJ_WORN(obj) || OBJ_CARRIED(obj))
						{
							tmpch = OBJ_WORN(obj) ? obj->loc.wearing :
										obj->loc.carrying;
							if (tmpch && IS_NPC(tmpch))
							{
								snprintf(
									buf, MAX_STRING_LENGTH,
									"  &+YFound another copy of arti '&n%s&+Y' &+w%d&+Y on '&n%s&+Y' &+w%d&+Y, pulling it.&n\n\r",
									OBJ_SHORT(arti),
									OBJ_VNUM(arti),
									J_NAME(tmpch),
									GET_VNUM(tmpch));
								send_to_char(buf, ch);
								extract_obj(obj, FALSE);
							}
						}
						else
						{
							snprintf(
								buf, MAX_STRING_LENGTH,
								"  &+YFound another copy of arti '&n%s&+Y' &+w%d&+Y in game not on a char, pulling it.&n\n\r",
								OBJ_SHORT(arti), OBJ_VNUM(arti));
							send_to_char(buf, ch);
							extract_obj(
								obj,
								TRUE); // Yes, we want to remove the arti data here.
						}
					}
				}
			}

			// If we can read everything, update the entry for it.
			artifact_update_sql(vnum, TRUE,
					    (temp == 0) ? ARTIFACT_ON_PC : ARTIFACT_ONCORPSE, pid,
					    timer, type);

			if (super)
			{
				owner = load_dummy_char(pname);
				if (!owner)
				{
					extract_obj(arti, FALSE);
				}
				else
				{
					bool owner_saved = TRUE;
					if (get_object_from_char(owner, vnum) == NULL)
					{
						obj_to_char(arti, owner);
						owner_saved = writeCharacter(owner, RENT_CRASH,
									     owner->in_room);
					}
					if (owner_saved)
					{
						nuke_eq(owner);
						extract_char(owner);
					}
					else
					{
						persistence_alert(AVATAR, "artifact",
								  "offline_owner", "none", "none",
								  "terminal_save_failed",
								  "extract_refused=1");
					}
				}
			}
			else
				extract_obj(arti, FALSE);
		}
	}

	closedir(dir);
}

void event_artifact_check_poof_sql(P_char /*ch*/, P_char /*vict*/, P_obj /*obj*/, void * /*arg*/)
{
	static int cursor_vnum = 0;
	P_obj arti, cont, corpse;
	P_char owner;
	P_desc desc;
	int vnum, locType, location;
	bool found;
	bool expired = FALSE;
	bool save_failed = FALSE;
	char *name;
#ifndef __NO_MYSQL__
	MYSQL_RES *res;
	MYSQL_ROW row = NULL;
#else
	flatfile_artifact_record flat_expired;
	const int64_t expiry_now = static_cast<int64_t>(time(NULL));
#endif
	size_t row_count;
	int page_last_vnum = cursor_vnum;

	if (!updateArtis)
	{
		cursor_vnum = 0;
		return;
	}

	// Each expired artifact can require global object scans or an offline player save.
	// Page by vnum so only a fixed amount of that work runs in one game pulse.
#ifdef __NO_MYSQL__
	std::string error;
	const auto selected = flatfile_artifact_find_next_expired(
		persistence_mode_flatfile_root(), cursor_vnum, expiry_now, &flat_expired, &error);
	if (selected == flatfile_artifact_result::not_found)
		row_count = 0;
	else if (selected == flatfile_artifact_result::ok)
		row_count = 1;
	else
	{
		logit(LOG_ARTIFACT, "event_artifact_check_poof_sql: flat artifact read failed: %s",
		      error.empty() ? "missing or invalid artifact authority" : error.c_str());
		nevent_periodic_mark_failure("artifact-expiry flat read failed");
		return;
	}
#else
	if (!qry("SELECT vnum, locType, location FROM artifacts WHERE owned='Y' AND timer < now() AND vnum > %d ORDER BY vnum LIMIT %zu",
		 cursor_vnum, ARTIFACT_EXPIRY_BATCH_SIZE))
	{
		logit(LOG_ARTIFACT, "event_artifact_check_poof_sql: failed to read from database.");
		nevent_periodic_mark_failure("artifact-expiry query failed");
		return;
	}
	res = mysql_store_result(DB);
	if (!res)
	{
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		nevent_periodic_mark_failure("artifact-expiry result failed");
		return;
	}
	row_count = static_cast<size_t>(mysql_num_rows(res));
#endif

	// If there were any artis to pull
	if (row_count > 0)
	{
		expired = TRUE;
		for (size_t row_index = 0; row_index < row_count; ++row_index)
		{
			bool owner_terminal_saved = TRUE;
#ifdef __NO_MYSQL__
			vnum = flat_expired.vnum;
			locType = flat_expired.location_type;
			location = flat_expired.location;
#else
			row = mysql_fetch_row(res);
			if (!row)
				break;
			vnum = atoi(row[0]);
			locType = atoi(row[1]);
			location = atoi(row[2]);
#endif
			page_last_vnum = vnum;

			// Not in game: nothing to find or poof, the UPDATE below clears the row.
			if (locType == ARTIFACT_NOTINGAME)
			{
				continue;
			}

			arti = NULL;
			if (locType == ARTIFACT_ONGROUND)
			{
				location = real_room(location);
				if (location >= 0 && location <= top_of_world)
				{
					for (arti = world[location].contents; arti;
					     arti = arti->next_content)
					{
						if (OBJ_VNUM(arti) == vnum)
						{
							break;
						}
					}
				}
				// Here things get hairy.  We need to find a copy of the arti that's on ground or in a container
				//   that's not on an Immortal or in a bag possessed by an Immortal.  This case should never come
				//   up, but just in case.
				if (!arti || OBJ_VNUM(arti) != vnum)
				{
					// Find the artifact in game, if it is.
					found = FALSE;
					for (arti = object_list; arti && !found; arti = arti->next)
					{
						if (OBJ_VNUM(arti) == vnum)
						{
							cont = arti;
							// Find outermost container.
							while (OBJ_INSIDE(cont) && cont->loc.inside)
							{
								cont = cont->loc.inside;
							}
							switch (cont->loc_p)
							{
							// Score!
							case LOC_ROOM:
								logit(LOG_ARTIFACT,
								      "event_artifact_check_poof_sql: Bad location on 'OnGround' arti(%d) - actual room v%d.",
								      vnum,
								      ROOM_VNUM(cont->loc.room));
								found = TRUE;
								break;
							case LOC_CARRIED:
							case LOC_WORN:
								owner = OBJ_WORN(cont) ?
										cont->loc.wearing :
										cont->loc.carrying;
								if (IS_NPC(owner))
								{
									logit(LOG_ARTIFACT,
									      "event_artifact_check_poof_sql: Bad location on 'OnGround' arti(%d) - on mob v%d.",
									      vnum,
									      GET_VNUM(owner));
									found = TRUE;
								}
								break;
							case LOC_INSIDE:
								logit(LOG_ARTIFACT,
								      "event_artifact_check_poof_sql: Bad loc_p (%d) on 'OnGround' arti(%d)-container vnum %d inside nothing.",
								      cont->loc_p, vnum,
								      OBJ_VNUM(cont));
								break;
							// Lost artis. :(
							case LOC_NOWHERE:
							default:
								logit(LOG_ARTIFACT,
								      "event_artifact_check_poof_sql: Bad loc_p (%d) on 'OnGround' arti(%d)-container vnum %d.",
								      cont->loc_p, vnum,
								      OBJ_VNUM(cont));
								break;
							}
						}
					}
					// Just to make sure.
					if (!found)
					{
						arti = NULL;
					}
				}
				if (!arti || OBJ_VNUM(arti) != vnum)
				{
					logit(LOG_ARTIFACT,
					      "event_artifact_check_poof_sql: Could not find 'OnGround' artifact vnum %d anywhere.",
					      vnum);
					continue;
				}
				// If we found arti in game.
				else
				{
					// Poof it!
					poof_artifact(arti);
				}
			} // End if locType == ARTIFACT_ONGROUND.
			else if (locType == ARTIFACT_ON_PC)
			{
				// Try to load the pfile if it's on a PC that isn't online.
				if (!is_pid_online(location, TRUE))
				{
					logit(LOG_ARTIFACT,
					      "event_artifact_check_poof_sql: poofing vnum=%d on offline pid=%d ('%s')",
					      vnum, location, get_player_name_from_pid(location));

					owner = load_dummy_char(get_player_name_from_pid(location));

					if (owner)
					{
						arti = get_object_from_char(owner, vnum);
					}
					else
					{
						logit(LOG_ARTIFACT,
						      "event_artifact_check_poof_sql: Could not load pfile of '%s' %d, to poof arti vnum %d.",
						      get_player_name_from_pid(location), location,
						      vnum);
						arti = NULL;
					}
					if (arti)
					{
						poof_artifact(arti);
						if (!writeCharacter(owner, RENT_POOFARTI,
								    owner->in_room))
						{
							save_failed = TRUE;
							owner_terminal_saved = FALSE;
							persistence_alert(AVATAR, "artifact",
									  "offline_poof", "none",
									  "none",
									  "terminal_save_failed",
									  "extract_refused=1");
						}
						logit(LOG_ARTIFACT,
						      "event_artifact_check_poof_sql: poofed vnum=%d for offline pid=%d ('%s')",
						      vnum, location,
						      get_player_name_from_pid(location));
					}
					else
					{
						if (owner)
						{
							// Nuke the eq off dummy char so it doesn't fall to the ground and get duped.
							nuke_eq(owner);
						}
						logit(LOG_ARTIFACT,
						      "event_artifact_check_poof_sql: Could not find artifact vnum %d on pfile of '%s' %d.",
						      vnum, get_player_name_from_pid(location),
						      location);
					}
					if (owner && owner_terminal_saved)
					{
						extract_char(owner);
					}
				}
				// PC online.
				else
				{
					arti = NULL;
					// First check descriptors (for speed).
					for (desc = descriptor_list; desc; desc = desc->next)
					{
						// Char must be in game and exist.
						if (desc->connected != CON_PLAYING ||
						    !(owner = GET_TRUE_CHAR_D(desc)))
						{
							continue;
						}
						// Skip immortals.
						if (IS_TRUSTED(owner))
						{
							continue;
						}
						if ((arti = get_object_from_char(owner, vnum)))
						{
							break;
						}
					}
					// If we didn't find it on descriptor list, gotta check character list for ld chars :(
					if (arti == NULL)
					{
						for (owner = character_list; owner;
						     owner = owner->next)
						{
							// Found them ld! Hopefully, there's only one of them.
							if (IS_PC(owner) &&
							    GET_PID(owner) == location)
							{
								arti = get_object_from_char(owner,
											    vnum);
								break;
							}
						}
					}
					if (arti != NULL)
					{
						poof_artifact(arti);
					}
					else
					{
						logit(LOG_ARTIFACT,
						      "event_artifact_check_poof_sql: Could not find artifact vnum %d on char '%s' %d.",
						      vnum, get_player_name_from_pid(location),
						      location);
					}
				}
			}
			else if (locType == ARTIFACT_ON_NPC)
			{
				for (owner = character_list; owner; owner = owner->next)
				{
					if (IS_NPC(owner) && GET_VNUM(owner) == location)
					{
						if ((arti = get_object_from_char(owner, vnum)))
						{
							break;
						}
					}
				}
				if (arti != NULL)
				{
					poof_artifact(arti);
				}
				else
				{
					logit(LOG_ARTIFACT,
					      "event_artifact_check_poof_sql: Could not find artifact vnum %d on mob %d.",
					      vnum, location);
				}
			}
			else if (locType == ARTIFACT_ONCORPSE)
			{
				name = get_player_name_from_pid(location);
				found = FALSE;
				corpse = NULL;
				for (arti = object_list; arti; arti = arti->next)
				{
					if (OBJ_VNUM(arti) == vnum)
					{
						cont = arti;
						// Find outermost container.
						while (OBJ_INSIDE(cont) && cont->loc.inside)
						{
							cont = cont->loc.inside;

							if (OBJ_VNUM(cont) == VOBJ_CORPSE &&
							    IS_SET(cont->value[CORPSE_FLAGS],
								   PC_CORPSE) &&
							    isname(name, cont->name))
							{
								corpse = cont;
								// It's on a corpse of the right char (don't stop moving outward though).
								found = TRUE;
							}
						}
						if (found)
						{
							obj_from_obj(arti);
							if (corpse != cont)
							{
								logit(LOG_ARTIFACT,
								      "event_artifact_check_poof_sql: Poofing arti from corpse inside a container. ugh.");
							}
							else
							{
								writeCorpse(corpse);
							}
							switch (cont->loc_p)
							{
							case LOC_ROOM:
								obj_to_room(arti, cont->loc.room);
								break;
							case LOC_CARRIED:
							case LOC_WORN:
								owner = OBJ_WORN(cont) ?
										cont->loc.wearing :
										cont->loc.carrying;
								obj_to_room(arti, owner->in_room);
								break;
							case LOC_INSIDE:
								logit(LOG_ARTIFACT,
								      "event_artifact_check_poof_sql: Bad loc_p (%d) on 'OnCorpse' arti(%d)-container vnum %d inside nothing.",
								      cont->loc_p, vnum,
								      OBJ_VNUM(cont));
								break;
							// Lost artis. :(
							case LOC_NOWHERE:
							default:
								logit(LOG_ARTIFACT,
								      "event_artifact_check_poof_sql: Bad loc_p (%d) on 'OnCorpse' arti(%d)-container vnum %d.",
								      cont->loc_p, vnum,
								      OBJ_VNUM(cont));
								break;
							}
							break;
						}
					}
				}
				if (found)
				{
					poof_artifact(arti);
					break;
				}
				else
				{
					// Check for arti in anyplace other than on an Immortal.
					for (arti = object_list; arti && !found; arti = arti->next)
					{
						if (OBJ_VNUM(arti) == vnum)
						{
							cont = arti;
							// Find outermost container.
							while (OBJ_INSIDE(cont) && cont->loc.inside)
							{
								cont = cont->loc.inside;
							}
							switch (cont->loc_p)
							{
							case LOC_ROOM:
								logit(LOG_ARTIFACT,
								      "event_artifact_check_poof_sql: Bad location on 'OnCorpse' arti(%d) - in room v%d.",
								      vnum,
								      ROOM_VNUM(cont->loc.room));
								found = TRUE;
								break;
							case LOC_CARRIED:
							case LOC_WORN:
								owner = OBJ_WORN(cont) ?
										cont->loc.wearing :
										cont->loc.carrying;
								if (IS_NPC(owner))
								{
									logit(LOG_ARTIFACT,
									      "event_artifact_check_poof_sql: Bad location on 'OnCorpse' arti(%d) - on mob v%d.",
									      vnum,
									      GET_VNUM(owner));
									found = TRUE;
								}
								break;
							case LOC_INSIDE:
								logit(LOG_ARTIFACT,
								      "event_artifact_check_poof_sql: Bad loc_p (%d) on 'OnCorpse' arti(%d)-container vnum %d inside nothing.",
								      cont->loc_p, vnum,
								      OBJ_VNUM(cont));
								break;
							// Lost artis. :(
							case LOC_NOWHERE:
							default:
								logit(LOG_ARTIFACT,
								      "event_artifact_check_poof_sql: Bad loc_p (%d) on 'OnCorpse' arti(%d)-container vnum %d.",
								      cont->loc_p, vnum,
								      OBJ_VNUM(cont));
								break;
							}
						}
					}
					if (found)
					{
						poof_artifact(arti);
					}
					else
					{
						logit(LOG_ARTIFACT,
						      "event_artifact_check_poof_sql: Could not find artifact vnum %d on PC corpse %s (%d).",
						      vnum, name, location);
					}
				}
			}
			else
			{
				logit(LOG_ARTIFACT,
				      "event_artifact_check_poof_sql: Could not find artifact vnum %d: bad locType: %d.",
				      vnum, locType);
			}
		}
	}

#ifndef __NO_MYSQL__
	mysql_free_result(res);
#endif

	// Clear only the page that was processed.  Keeping this after the loop preserves
	// the all-or-nothing behavior for offline owner saves within the page.
	if (expired && !save_failed)
	{
#ifdef __NO_MYSQL__
		const auto cleared = flatfile_artifact_expire(persistence_mode_flatfile_root(),
							      page_last_vnum, expiry_now, &error);
		if (cleared == flatfile_artifact_result::ok)
			arti_cache_invalidate();
		else if (cleared != flatfile_artifact_result::unchanged)
		{
			nevent_periodic_retry_after(ARTIFACT_MAINTENANCE_RETRY_DELAY,
						    "artifact-expiry flat update failed");
			return;
		}
#else
		if (qry("UPDATE artifacts SET owned='N', locType=%d, location=-1, timer=NULL, lastUpdate=SYSDATE() WHERE owned='Y' AND timer < now() AND vnum = %d",
			ARTIFACT_NOTINGAME, page_last_vnum))
			arti_cache_invalidate();
		else
		{
			nevent_periodic_retry_after(ARTIFACT_MAINTENANCE_RETRY_DELAY,
						    "artifact-expiry update failed");
			return;
		}
#endif
	}
	else if (save_failed)
	{
		nevent_periodic_retry_after(ARTIFACT_MAINTENANCE_RETRY_DELAY,
					    "artifact-expiry player save failed");
		return;
	}

	if (row_count == ARTIFACT_EXPIRY_BATCH_SIZE)
	{
		cursor_vnum = page_last_vnum;
		nevent_periodic_continue_after(1);
		return;
	}
	cursor_vnum = 0;
}

void event_artifact_wars_sql(P_char /*ch*/, P_char /*vict*/, P_obj /*obj*/, void * /*arg*/)
{
	struct artifact_wars_owner
	{
		int pid;
		int total;
		int major;
		int unique;
		int ioun;
	};
	static int cursor_pid = 0;
	artifact_wars_owner owners[ARTIFACT_WARS_OWNER_BATCH_SIZE] = {};
	size_t owner_count = 0;
	bool timers_updated = false;
	bool update_failed = false;
#ifndef __NO_MYSQL__
	MYSQL_RES *res;
	MYSQL_ROW row;
#endif

	if (!updateArtis)
	{
		cursor_pid = 0;
		return;
	}

	debug("event_artifact_wars: beginning...");

	// Fraction of an artifact's remaining life burned off per level over the
	// one-per-type limit.  0.0 (the default when the property is absent)
	// leaves the timers alone and the forced drop below is the whole penalty.
	float modifier = get_property("artifact.wars.modifier", 0.0);
	if (modifier < 0.0f)
		modifier = 0.0f;
	if (modifier > 1.0f)
		modifier = 1.0f;

	// Aggregate only violating owners and page them by pid.  Timer updates are one
	// statement per owner, so the callback never materializes the whole artifact table.
	debug("event_artifact_wars_sql: querying artifact owners after pid %d...", cursor_pid);
#ifdef __NO_MYSQL__
	std::vector<flatfile_artifact_war_owner> flat_owners;
	std::string error;
	if (flatfile_artifact_war_owners(persistence_mode_flatfile_root(), cursor_pid,
					 ARTIFACT_WARS_OWNER_BATCH_SIZE, &flat_owners,
					 &error) != flatfile_artifact_result::ok)
	{
		logit(LOG_ARTIFACT, "event_artifact_wars_sql: flat artifact read failed: %s",
		      error.empty() ? "missing or invalid artifact authority" : error.c_str());
		nevent_periodic_retry_after(ARTIFACT_MAINTENANCE_RETRY_DELAY,
					    "artifact-wars flat read failed");
		return;
	}
	owner_count = flat_owners.size();
	for (size_t index = 0; index < owner_count; ++index)
	{
		owners[index] = { flat_owners[index].pid, flat_owners[index].total,
				  flat_owners[index].major, flat_owners[index].unique,
				  flat_owners[index].ioun };
	}
#else
	if (!qry("SELECT location, COUNT(*), SUM(type=%d), SUM(type=%d), SUM(type=%d) FROM artifacts WHERE locType=%d AND location > %d GROUP BY location HAVING SUM(type=%d) > 1 OR SUM(type=%d) > 1 OR SUM(type=%d) > 1 ORDER BY location LIMIT %zu",
		 ARTIFACT_MAJOR, ARTIFACT_UNIQUE, ARTIFACT_IOUN, ARTIFACT_ON_PC, cursor_pid,
		 ARTIFACT_MAJOR, ARTIFACT_UNIQUE, ARTIFACT_IOUN, ARTIFACT_WARS_OWNER_BATCH_SIZE))
	{
		logit(LOG_ARTIFACT, "event_artifact_wars_sql: failed to read from database.");
		nevent_periodic_retry_after(ARTIFACT_MAINTENANCE_RETRY_DELAY,
					    "artifact-wars query failed");
		return;
	}
	res = mysql_store_result(DB);

	if (!res)
	{
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		nevent_periodic_retry_after(ARTIFACT_MAINTENANCE_RETRY_DELAY,
					    "artifact-wars result failed");
		return;
	}

	while (owner_count < ARTIFACT_WARS_OWNER_BATCH_SIZE && (row = mysql_fetch_row(res)))
	{
		artifact_wars_owner &entry = owners[owner_count++];
		entry.pid = atoi(row[0]);
		entry.total = atoi(row[1]);
		entry.major = row[2] ? atoi(row[2]) : 0;
		entry.unique = row[3] ? atoi(row[3]) : 0;
		entry.ioun = row[4] ? atoi(row[4]) : 0;
	}
	mysql_free_result(res);
#endif

	if (owner_count == 0)
	{
		cursor_pid = 0;
		debug("event_artifact_wars_sql: No violating artifact owners found.");
		return;
	}

	debug("event_artifact_wars_sql: Found %zu violating owners, processing...", owner_count);
	for (size_t index = 0; index < owner_count; ++index)
	{
		const artifact_wars_owner &entry = owners[index];
		int punish_level = entry.major > 1 ? entry.major - 1 : 0;
		punish_level += entry.unique > 1 ? entry.unique - 1 : 0;
		punish_level += entry.ioun > 1 ? entry.ioun - 1 : 0;

		// Burn time while the rows still identify their owner; dropping the objects may
		// immediately update their database location.
		if (modifier > 0.0f)
		{
			float burn = modifier * (float)punish_level;
			const time_t now = time(NULL);
			if (burn > 1.0f)
				burn = 1.0f;
			const float retained = 1.0f - burn;
#ifdef __NO_MYSQL__
			const auto updated = flatfile_artifact_apply_war_burn(
				persistence_mode_flatfile_root(), entry.pid,
				static_cast<int64_t>(now), retained, static_cast<int64_t>(now),
				&error);
			if (updated == flatfile_artifact_result::ok)
			{
				timers_updated = true;
				logit(LOG_ARTIFACT,
				      "artifact_wars: pid %d artifact timers cut by %d%% (punish_level=%d)",
				      entry.pid, (int)(burn * 100.0f), punish_level);
			}
			else if (updated != flatfile_artifact_result::unchanged)
				update_failed = true;
#else
			if (qry("UPDATE artifacts SET timer = FROM_UNIXTIME(%lu + FLOOR((UNIX_TIMESTAMP(timer) - %lu) * %.9f)), lastUpdate=SYSDATE() WHERE locType=%d AND location=%d AND timer > FROM_UNIXTIME(%lu)",
				(unsigned long)now, (unsigned long)now, retained, ARTIFACT_ON_PC,
				entry.pid, (unsigned long)now))
			{
				timers_updated = true;
				logit(LOG_ARTIFACT,
				      "artifact_wars: pid %d artifact timers cut by %d%% (punish_level=%d)",
				      entry.pid, (int)(burn * 100.0f), punish_level);
			}
			else
				update_failed = true;
#endif
		}

		P_char owner = find_player_by_pid(entry.pid);
		if (owner && owner->in_room != NOWHERE)
		{
			P_obj carried_obj, next_obj;
			bool first = TRUE;

			for (int pos = 0; pos < MAX_WEAR; pos++)
			{
				if (owner->equipment[pos] && IS_ARTIFACT(owner->equipment[pos]))
				{
					carried_obj = unequip_char(owner, pos, FALSE);
					if (first)
					{
						act("&+RThe gods frown upon your greed! $p burns your flesh and falls to the ground!&n",
						    FALSE, owner, carried_obj, 0, TO_CHAR);
						act("&+R$n screams as $p burns $m and falls to the ground!&n",
						    FALSE, owner, carried_obj, 0, TO_ROOM);
						first = FALSE;
					}
					else
					{
						act("&+R$p falls to the ground!&n", FALSE, owner,
						    carried_obj, 0, TO_CHAR);
						act("&+R$p falls to the ground!&n", FALSE, owner,
						    carried_obj, 0, TO_ROOM);
					}
					obj_to_room(carried_obj, owner->in_room);
				}
			}

			for (carried_obj = owner->carrying; carried_obj; carried_obj = next_obj)
			{
				next_obj = carried_obj->next_content;
				if (!IS_ARTIFACT(carried_obj))
					continue;
				if (first)
				{
					act("&+RThe gods frown upon your greed! $p burns your flesh and falls to the ground!&n",
					    FALSE, owner, carried_obj, 0, TO_CHAR);
					act("&+R$n screams as $p burns $m and falls to the ground!&n",
					    FALSE, owner, carried_obj, 0, TO_ROOM);
					first = FALSE;
				}
				else
				{
					act("&+R$p falls to the ground!&n", FALSE, owner,
					    carried_obj, 0, TO_CHAR);
					act("&+R$p falls to the ground!&n", FALSE, owner,
					    carried_obj, 0, TO_ROOM);
				}
				obj_from_char(carried_obj);
				obj_to_room(carried_obj, owner->in_room);
			}

			debug("artifact_wars: %s had %d artifacts forcibly dropped (punish_level=%d)",
			      GET_NAME(owner), entry.total, punish_level);
		}
	}

	if (timers_updated)
		arti_cache_invalidate();
	if (update_failed)
		nevent_periodic_mark_failure("artifact-wars timer update failed");

	cursor_pid = owners[owner_count - 1].pid;
	if (owner_count == ARTIFACT_WARS_OWNER_BATCH_SIZE)
	{
		nevent_periodic_continue_after(1);
		return;
	}
	cursor_pid = 0;
	debug("event_artifact_wars: ended.");
}

void event_arti_hunt_sql(P_char ch, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	if (!IS_ALIVE(ch) || !data)
	{
		statuslog(56, "event_arti_hunt_sql: bad arg: ch '%s', data '%s'",
			  ch ? J_NAME(ch) : "NULL", data != NULL ? (char *)data : "NULL");
		debug("event_arti_hunt_sql: bad arg: ch '%s', data '%s'", ch ? J_NAME(ch) : "NULL",
		      data != NULL ? (char *)data : "NULL");
		return;
	}

	arti_hunt_sql(ch, (char *)data);
}

// Searches through all pfiles with initial *arg for artis.
void arti_hunt_sql(P_char ch, const char *arg)
{
	char buf[MAX_STRING_LENGTH];
	char dname[256];
	char initial;
	int count, wearloc;
	arti_data artidata;
	struct dirent *dire;
	DIR *dir;
	P_char owner, mob;
	P_obj arti, arti2;

	if (atoi(arg) == 1)
	{
		// Search for a without delay.
		arti_hunt_sql(ch, "a");
		// For the rest of the letters search for them with an incremented delay to prevent lag.
		for (initial = 'b', count = 1; initial <= 'z'; initial++, count++)
		{
			snprintf(buf, MAX_STRING_LENGTH, "%c", initial);
			add_event(event_arti_hunt_sql, count, ch, NULL, NULL, 0, &buf, sizeof(buf));
		}
		return;
	}
	if (!*arg || !isalpha(*arg))
	{
		send_to_char(
			"Arti hunt needs a letter for which initial to hunt for, or 1 to search all pfiles.\n",
			ch);
		send_to_char(
			"Arti hunt will then search all pfiles with said initial for artifacts, and add them to the arti list if necessary.\n",
			ch);
		return;
	}
	// Save directories are lower case; don't lower-case the caller's buffer.
	initial = (*arg >= 'A' && *arg <= 'Z') ? (char)(*arg + 'a' - 'A') : *arg;

	// Read & loop through the directory..
	// Open the directory!
	snprintf(dname, 256, "%s/%c", SAVE_DIR, initial);
	dir = opendir(dname);
	if (!dir)
	{
		statuslog(56, "hunt_for_artis: could not open arti dir (%s)\r\n", ARTIFACT_DIR);
		debug("hunt_for_artis: could not open arti dir (%s)\r\n", ARTIFACT_DIR);
		return;
	}

	// Loop through the directory files.
	while ((dire = readdir(dir)))
	{
		// Skip backup/locker files/etc
		if (strstr(dire->d_name, "."))
			continue;

		if ((owner = load_dummy_char(dire->d_name)) == NULL)
		{
			snprintf(buf, MAX_STRING_LENGTH, "hunt_for_artis: %s has bad pfile.\n\r",
				 dire->d_name);
			send_to_char(buf, ch);
			continue;
		}
		if (IS_TRUSTED(owner))
		{
			nuke_eq(owner);
			extract_char(owner);
			continue;
		}

		/* For debugging only.. gets spammy on live mud.
		snprintf(buf, MAX_STRING_LENGTH, "Hunting pfile of '%s'.\n", J_NAME(owner) );
		send_to_char( buf, ch );
		*/

		// Search each pfile:
		// Search Worn equipment.
		for (wearloc = 0; wearloc < MAX_WEAR; wearloc++)
		{
			arti = owner->equipment[wearloc];
			if (arti == NULL || !IS_ARTIFACT(arti))
			{
				continue;
			}

			snprintf(buf, MAX_STRING_LENGTH, "%-12s has %s&n (%6d) : ", J_NAME(owner),
				 pad_ansi(arti->short_description, 35, TRUE).c_str(),
				 OBJ_VNUM(arti));
			send_to_char(buf, ch);

			if (!get_artifact_data_sql(OBJ_VNUM(arti), &artidata))
			{
				send_to_char("&+WNot yet tracked - adding.&n\n", ch);
				// If there's one in zone, pull it.
				if ((arti2 = artifact_find(OBJ_VNUM(arti))))
				{
					send_to_char("&+WPulled artifact from zone.\n\r", ch);
					extract_obj(arti2);
				}
				// If they managed to get it on pfile and not in DB, give them full timer.
				artifact_update_sql(arti, 'Y',
						    time(NULL) + ARTIFACT_BLOOD_DAYS *
									 SECS_PER_REAL_DAY);
			}
			else if (artidata.locType == ARTIFACT_ON_PC ||
				 artidata.locType == ARTIFACT_ONCORPSE)
			{
				if (artidata.location == GET_PID(owner))
				{
					send_to_char("Already tracked on char.\n", ch);
				}
				else
				{
					snprintf(buf, MAX_STRING_LENGTH,
						 "&+ROn another char:&N %s\n",
						 get_player_name_from_pid(artidata.location));
					send_to_char(buf, ch);
				}
			}
			else if (artidata.locType == ARTIFACT_ON_NPC)
			{
				mob = read_mobile(artidata.location, VIRTUAL);
				snprintf(buf, MAX_STRING_LENGTH, "&+ROn a mob:&N '%s' %d.\n",
					 J_NAME(mob), artidata.location);
				extract_char(mob);
				send_to_char(buf, ch);
				// If there's one in zone, pull it.
				if ((arti2 = artifact_find(OBJ_VNUM(arti))))
				{
					send_to_char("&+WPulled artifact from zone.\n\r", ch);
					extract_obj(arti2);
				}
				artifact_update_location_sql(arti);
			}
			else if (artidata.locType == ARTIFACT_ONGROUND)
			{
				snprintf(buf, MAX_STRING_LENGTH, "&+ROn ground:&N '%s' %d.\n",
					 world[real_room0(artidata.location)].name,
					 artidata.location);
				send_to_char(buf, ch);
				// If there's one in zone, pull it.
				if ((arti2 = artifact_find(OBJ_VNUM(arti))))
				{
					send_to_char("&+WPulled artifact from zone.\n\r", ch);
					extract_obj(arti2);
				}
				artifact_update_location_sql(arti);
			}
			else if (artidata.locType == ARTIFACT_NOTINGAME)
			{
				send_to_char("&+WNot in game - updating.&n\n\r", ch);
				artifact_update_location_sql(arti);
			}
			else
			{
				send_to_char("&+rUnknown location.&n\n\r", ch);
			}
		}
		// Search inventory.
		for (arti = owner->carrying; arti; arti = arti->next_content)
		{
			if (IS_ARTIFACT(arti))
			{
				snprintf(buf, MAX_STRING_LENGTH,
					 "%-12s has %s&n (%6d) : ", J_NAME(owner),
					 pad_ansi(arti->short_description, 35, TRUE).c_str(),
					 obj_index[arti->R_num].virtual_number);
				send_to_char(buf, ch);
				if (!get_artifact_data_sql(OBJ_VNUM(arti), &artidata))
				{
					send_to_char("&+WNot yet tracked - adding.&n\n", ch);
					// If they managed to get it on pfile and not in DB, give them full timer.
					artifact_update_sql(arti, 'Y',
							    time(NULL) + ARTIFACT_BLOOD_DAYS *
										 SECS_PER_REAL_DAY);
					// If there's one in zone, pull it.
					if ((arti2 = artifact_find(OBJ_VNUM(arti))))
					{
						send_to_char("&+WPulled artifact from zone.\n\r",
							     ch);
						extract_obj(arti2);
					}
				}
				else if (artidata.locType == ARTIFACT_ON_PC ||
					 artidata.locType == ARTIFACT_ONCORPSE)
				{
					if (artidata.location == GET_PID(owner))
					{
						send_to_char("Already tracked on char.\n", ch);
					}
					else
					{
						snprintf(buf, MAX_STRING_LENGTH,
							 "&+ROn another char:&N %s\n",
							 get_player_name_from_pid(
								 artidata.location));
						send_to_char(buf, ch);
					}
				}
				else if (artidata.locType == ARTIFACT_ON_NPC)
				{
					mob = read_mobile(artidata.location, VIRTUAL);
					snprintf(buf, MAX_STRING_LENGTH,
						 "&+ROn a mob:&N '%s' %d.\n", J_NAME(mob),
						 artidata.location);
					extract_char(mob);
					send_to_char(buf, ch);
				}
				else if (artidata.locType == ARTIFACT_ONGROUND)
				{
					snprintf(buf, MAX_STRING_LENGTH,
						 "&+ROn ground:&N '%s' %d.\n",
						 world[real_room0(artidata.location)].name,
						 artidata.location);
					send_to_char(buf, ch);
				}
				else if (artidata.locType == ARTIFACT_NOTINGAME)
				{
					send_to_char("&+rNot in game - updating.&n\n\r", ch);
					artifact_update_location_sql(arti);
				}
				else
				{
					send_to_char("&+rUnknown location.&n\n\r", ch);
				}
			}
		}
		nuke_eq(owner);
		extract_char(owner);
	}
	// Close the directory!
	closedir(dir);
	snprintf(buf, MAX_STRING_LENGTH, "Arti hunted '%c' successfully!\n", *arg);
	send_to_char(buf, ch);
}

void arti_clear_sql(P_char ch, char *arg)
{
	int vnum;
	P_obj arti;

	if (!*arg || !strcmp(arg, "?") || !strcmp(arg, "help"))
	{
		send_to_char("This command clears the arti data for a specific arti vnum.\n\r", ch);
		send_to_char("It should only be used by those who know what they're doing.\n\r",
			     ch);
		send_to_char(
			"This will delete the entire entry from the Mortal and Immortal DB, be forewarned.\n\r",
			ch);
		return;
	}
	if (GET_LEVEL(ch) < FORGER)
	{
		send_to_char("Maybe you could ask someone of higher level to do this.\n\r", ch);
		return;
	}

	if ((vnum = atoi(arg)) <= 0)
	{
		send_to_char(
			"The proper argument for this is the vnum of the arti for which you want the data cleared.\n\r",
			ch);
		return;
	}

	if (!(arti = read_object(vnum, VIRTUAL)))
	{
		send_to_char("&+WThat's not a vnum for any object, wth?&n\n\r", ch);
		return;
	}
	if (!IS_ARTIFACT(arti))
	{
		act("$p &+Wis not an artifact.", FALSE, ch, arti, 0, TO_CHAR);
	}

#ifdef __NO_MYSQL__
	std::string error;
	const auto removed =
		flatfile_artifact_erase(persistence_mode_flatfile_root(), vnum, &error);
	if (removed == flatfile_artifact_result::ok)
	{
		arti_cache_invalidate();
		act("&+WThe artifact data for $p&+W has been cleared.  You fool!", FALSE, ch, arti,
		    0, TO_CHAR);
	}
	else
	{
		logit(LOG_ARTIFACT, "arti_clear_sql: flat artifact erase failed for %d: %s", vnum,
		      error.empty() ? "missing or invalid artifact authority" : error.c_str());
		send_to_char("&+WFailed to remove entry from artifact data.&n\n\r", ch);
	}
#else
	// Remove from artifacts table:
	if (qry("DELETE FROM artifacts WHERE vnum = '%d'", vnum))
	{
		arti_cache_invalidate();
		act("&+WThe artifact data for $p&+W has been cleared from the Immortal list.  You fool!",
		    FALSE, ch, arti, 0, TO_CHAR);
		if (qry("DELETE FROM artifacts_mortal WHERE vnum = '%d'", vnum))
		{
			act("&+WThe artifact data for $p&+W has been cleared from the Mortal list.  You fool!",
			    FALSE, ch, arti, 0, TO_CHAR);
		}
		else
		{
			send_to_char("&+WFailed to remove entry from mortal DB.&n\n\r", ch);
		}
	}
	else
	{
		send_to_char("&+WFailed to remove entry from main DB.  wth?&n\n\r", ch);
	}
#endif
	extract_obj(arti);
}

// This function is used to poof an arti that's either in game or on a rented char.
// If the timer isn't ticking, this function won't do anything.
void arti_poof_sql(P_char ch, char *arg)
{
	char buf[MAX_STRING_LENGTH], artishort[MAX_STRING_LENGTH];
	int vnum;
	P_obj arti;
	P_char owner;
	arti_data artidata;

	if (!*arg || !strcmp(arg, "?") || !strcmp(arg, "help"))
	{
		send_to_char("This command poofs the artifact specified by the vnum argument.\n\r",
			     ch);
		send_to_char("It should only be used by those who know what they're doing.\n\r",
			     ch);
		send_to_char("This will poof the artifact and reset the timer, be forewarned.\n\r",
			     ch);
		return;
	}
	if (GET_LEVEL(ch) < FORGER)
	{
		send_to_char("Maybe you could ask someone of higher level to do this.\n\r", ch);
		return;
	}

	if ((vnum = atoi(arg)) <= 0)
	{
		send_to_char(
			"The proper argument for this is the vnum of the arti for which you want the data cleared.\n\r",
			ch);
		return;
	}

	if (!(arti = read_object(vnum, VIRTUAL)))
	{
		send_to_char("&+WThat's not a vnum for any object, wth?&n\n\r", ch);
		return;
	}
	if (!IS_ARTIFACT(arti))
	{
		act("$p &+Wis not an artifact.", FALSE, ch, arti, 0, TO_CHAR);
		extract_obj(arti);
		return;
	}
	snprintf(artishort, MAX_STRING_LENGTH, "%s&n", OBJ_SHORT(arti));
	extract_obj(arti);

	if (!get_artifact_data_sql(vnum, &artidata))
	{
		checked_snprintf(
			buf, MAX_STRING_LENGTH,
			"&+WThere is no data for %s atm; It shouldn't be in the game.&n\n\r",
			artishort);
		send_to_char(buf, ch);
		return;
	}
	owner = NULL;
	// If we can't find it in game.
	if (!(arti = artifact_find(artidata)))
	{
		// If it isn't on a char, or the char is online.
		if (artidata.locType != ARTIFACT_ON_PC || is_pid_online(artidata.location, TRUE))
		{
			checked_snprintf(buf, MAX_STRING_LENGTH, "Could not find %s in game.\n\r",
					 artishort);
			send_to_char(buf, ch);
			return;
		}
		if (!(owner = load_dummy_char(get_player_name_from_pid(artidata.location))))
		{
			snprintf(buf, MAX_STRING_LENGTH, "Could not load pfile of %s.\n\r",
				 get_player_name_from_pid(artidata.location));
			send_to_char(buf, ch);
			return;
		}
		if ((arti = get_object_from_char(owner, vnum)) == NULL)
		{
			checked_snprintf(buf, MAX_STRING_LENGTH,
					 "Strange, arti '%s' %d was not on %s's pfile!\n\r",
					 artishort, vnum,
					 get_player_name_from_pid(artidata.location));
			nuke_eq(owner);
			extract_char(owner);
			return;
		}
	}
	poof_artifact(arti);
	// If arti was on rented character.
	if (owner)
	{
		nuke_eq(owner);
		extract_char(owner);
	}
}

#define COMMAND_ADD 1
#define COMMAND_SUB 2
#define COMMAND_SET 3
// Changes the timer on an artifact.
void arti_timer_sql(P_char ch, char *arg)
{
	char buf[MAX_STRING_LENGTH], artishort[256];
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
	int minutes, cmd, vnum;
	time_t new_time;
	P_obj arti;
	arti_data artidata;

	arg = one_argument(arg, arg1);
	arg = one_argument(arg, arg2);
	arg = one_argument(arg, arg3);

	// If they don't have 3 args, then we can't do anything, so show the help.
	if (!*arg3)
	{
		send_to_char(
			"&+WFormat: &+wartifact timer <set|add|subtract> <vnum> <time in minutes>&+W.&n\n\r",
			ch);
		send_to_char(
			"This command changes the timer on the artifact specified by the vnum argument.\n\r",
			ch);
		send_to_char(
			"&+WIt should only be used by those who know what they're doing.&n\n\r",
			ch);
		send_to_char(
			"This will only work on artifacts who's timers are already ticking.\n\r",
			ch);
		return;
	}

	if (GET_LEVEL(ch) < GREATER_G)
	{
		send_to_char("Maybe you could ask someone of higher level to do this.\n\r", ch);
		return;
	}

	// Handle arg1: the command.
	if (is_abbrev(arg1, "set"))
	{
		cmd = COMMAND_SET;
	}
	else if (is_abbrev(arg1, "add"))
	{
		cmd = COMMAND_ADD;
	}
	else if (is_abbrev(arg1, "subtract"))
	{
		cmd = COMMAND_SUB;
	}
	else
	{
		send_to_char(
			"&+WFormat: &+wartifact timer <set|add|subtract> <vnum> <time in minutes>&+W.&n\n\r",
			ch);
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+W'&+w%s&+W' is not a valid subcommand. Please choose set, add or subtract.&n\n\r",
			arg1);
		send_to_char(buf, ch);
		return;
	}

	// Handle arg2: the vnum.
	if ((vnum = atoi(arg2)) <= 0)
	{
		send_to_char(
			"&+WFormat: &+wartifact timer <set|add|subtract> <vnum> <time in minutes>&+W.&n\n\r",
			ch);
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+W'&+w%s&+W' is not a valid vnum.  Please use a positive number for the vnum.&n\n\r",
			arg2);
		send_to_char(buf, ch);
		return;
	}
	if ((arti = read_object(vnum, VIRTUAL)) == NULL)
	{
		send_to_char(
			"&+WFormat: &+wartifact timer <set|add|subtract> <vnum> <time in minutes>&+W.&n\n\r",
			ch);
		snprintf(buf, MAX_STRING_LENGTH,
			 "&+W'&+w%s&+W' is not the vnum of any object in the game.&n\n\r", arg2);
		send_to_char(buf, ch);
		return;
	}
	if (!IS_ARTIFACT(arti))
	{
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+W'&+w%s&+W' &+w%d&+W is not an artifact.  This command only works with artifacts.&n\n\r",
			OBJ_SHORT(arti), vnum);
		send_to_char(buf, ch);
		return;
	}
	snprintf(artishort, 256, "%s&n", OBJ_SHORT(arti));
	extract_obj(arti);

	// Handle arg3: the time in minutes.
	if ((minutes = atoi(arg3)) == 0)
	{
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+W'%s' is not a positive number.  Please supply a positive number of minutes.&n\n\r",
			arg3);
		send_to_char(buf, ch);
		return;
	}
	if (minutes < 0)
	{
		if (!strcmp(arg1, "add"))
		{
			snprintf(
				buf, MAX_STRING_LENGTH,
				"&+WMaybe you should try '&+wartifact timer subtract %d %d&+W'.&n\n\r",
				vnum, minutes * -1);
			send_to_char(buf, ch);
		}
		else if (!strcmp(arg1, "subtract"))
		{
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+WMaybe you should try '&+wartifact timer add %d %d&+W'.&n\n\r",
				 vnum, minutes * -1);
			send_to_char(buf, ch);
		}
		else
		{
			send_to_char("&+WPlease supply a positive number of minutes.&n\n\r", ch);
		}
		return;
	}

	// Now we have a valid command, the vnum of an arti, and a postitive number of minutes to change.
	if (!get_artifact_data_sql(vnum, &artidata))
	{
		snprintf(buf, MAX_STRING_LENGTH,
			 "&+WHmm.. There's no timer ticking on '&+w%s&+W' &+w%d&+W.&n\n\r",
			 artishort, vnum);
		send_to_char(buf, ch);
		return;
	}

	// Calculate the new time at which to poof.
	if (cmd == COMMAND_ADD)
	{
		new_time = artidata.timer + 60 * minutes;
	}
	else if (cmd == COMMAND_SUB)
	{
		new_time = artidata.timer - 60 * minutes;
	}
	else if (cmd == COMMAND_SET)
	{
		new_time = time(NULL) + 60 * minutes;
	}
	else
	{
		send_to_char("&+WSomeone messed with this and created a faulty command. sorry.\n\r",
			     ch);
		return;
	}

	// Minimum of 1 minute.
	if (new_time < time(NULL))
	{
		send_to_char(
			"&+WNew time is less than &+w0&+W minutes, setting to &+w1&+W minute.&n\n\r",
			ch);
		new_time = time(NULL) + 60;
	}
	// And max.
	if (new_time > time(NULL) + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY)
	{
		send_to_char("&+WOops, went a little over max, didn't ya?&n\n\r", ch);
		new_time = time(NULL) + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
	}

	snprintf(buf, MAX_STRING_LENGTH,
		 "&+WArtifact '&+w%s&+W' &+w%d&+W has had it's timer changed from &n", artishort,
		 vnum);
	artifact_timer_sql(vnum, buf + strlen(buf));
	strcat(buf, "&+W to &n");

	// Use the uber-generic update.
	artifact_update_sql(vnum, artidata.owned, artidata.locType, artidata.location, new_time,
			    artidata.type);

	artifact_timer_sql(vnum, buf + strlen(buf));
	strcat(buf, "&+W.&n\n\r");

	send_to_char(buf, ch);
}

// This function is designed to swap out one arti for another.
void arti_swap_sql(P_char ch, char *arg)
{
	char buf[MAX_STRING_LENGTH];
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	char artishort1[256];
	int vnum1, vnum2, wearloc;
	bool found;
	P_obj arti1, arti2, cont;
	P_char owner1 = NULL, dummy;
	arti_data artidata;

	arg = one_argument(arg, arg1);
	arg = one_argument(arg, arg2);

	// If they don't have 2 args, then we can't do anything, so show the help.
	if (!*arg2)
	{
		send_to_char("&+WFormat: &+wartifact swap <vnum1> <vnum2>&+W.&n\n\r", ch);
		send_to_char("This command changes one artifact for another.\n\r", ch);
		send_to_char(
			"&+WIt should only be used by those who know what they're doing.&n\n\r",
			ch);
		send_to_char(
			"This will only work if arti1 has a ticking timer and arti2 doesn't.\n\r",
			ch);
		return;
	}

	// Handle arg1: the vnum of the first arti.
	if ((vnum1 = atoi(arg1)) <= 0)
	{
		send_to_char("&+WFormat: &+wartifact swap <vnum1> <vnum2>&+W.&n\n\r", ch);
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+W'&+w%s&+W' is not a valid vnum.  Please use a positive number for the vnum.&n\n\r",
			arg1);
		send_to_char(buf, ch);
		return;
	}
	if ((arti1 = read_object(vnum1, VIRTUAL)) == NULL)
	{
		send_to_char("&+WFormat: &+wartifact swap <vnum1> <vnum2>&+W.&n\n\r", ch);
		snprintf(buf, MAX_STRING_LENGTH,
			 "&+W'&+w%s&+W' is not the vnum of any object in the game.&n\n\r", arg1);
		send_to_char(buf, ch);
		return;
	}
	if (!IS_ARTIFACT(arti1))
	{
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+W'&+w%s&+W' &+w%d&+W is not an artifact.  This command only works with artifacts.&n\n\r",
			OBJ_SHORT(arti1), vnum1);
		send_to_char(buf, ch);
		return;
	}
	snprintf(artishort1, 256, "%s&n", OBJ_SHORT(arti1));
	extract_obj(arti1);

	// Handle arg2: the vnum of the first arti.
	if ((vnum2 = atoi(arg2)) <= 0)
	{
		send_to_char("&+WFormat: &+wartifact swap <vnum1> <vnum2>&+W.&n\n\r", ch);
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+W'&+w%s&+W' is not a valid vnum.  Please use a positive number for the vnum.&n\n\r",
			arg2);
		send_to_char(buf, ch);
		return;
	}
	if ((arti2 = read_object(vnum2, VIRTUAL)) == NULL)
	{
		send_to_char("&+WFormat: &+wartifact swap <vnum1> <vnum2>&+W.&n\n\r", ch);
		snprintf(buf, MAX_STRING_LENGTH,
			 "&+W'&+w%s&+W' is not the vnum of any object in the game.&n\n\r", arg2);
		send_to_char(buf, ch);
		return;
	}
	if (!IS_ARTIFACT(arti2))
	{
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+W'&+w%s&+W' &+w%d&+W is not an artifact.  This command only works with artifacts.&n\n\r",
			OBJ_SHORT(arti2), vnum2);
		send_to_char(buf, ch);
		return;
	}
	// Do not pull arti2.. will use it.

	// Now we have 2 valid vnums and their corresponding short descriptions.
	if (!get_artifact_data_sql(vnum1, &artidata))
	{
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+WThere's no timer ticking on '&+w%s&+W' &+w%d&+W. That won't work.&n\n\r",
			artishort1, vnum1);
		send_to_char(buf, ch);
		return;
	}
	// We just want to make sure it's not ticking.
	if (get_artifact_data_sql(vnum2, NULL))
	{
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+WThere's a timer ticking on '&+w%s&+W' &+w%d&+W.  That won't work.&n\n\r",
			OBJ_SHORT(arti2), vnum2);
		send_to_char(buf, ch);
		return;
	}
	// If there's a copy of arti2 in zone, pull it.
	if ((arti1 = artifact_find(vnum2)) != NULL)
	{
		extract_obj(arti1);
		snprintf(buf, MAX_STRING_LENGTH,
			 "&+WPulled artifact '&+w%s&+W' &+w%d&+W from zone.&n\n\r",
			 OBJ_SHORT(arti2), vnum2);
		send_to_char(buf, ch);
	}

	cont = NULL;
	for (arti1 = object_list; arti1; arti1 = arti1->next)
	{
		// Found it in game!
		if (OBJ_VNUM(arti1) == vnum1)
		{
			cont = arti1;
			while (OBJ_INSIDE(cont) && cont->loc.inside)
			{
				cont = cont->loc.inside;
			}
			found = FALSE;
			// Just make sure it's in a valid location.
			switch (cont->loc_p)
			{
			case LOC_CARRIED:
			case LOC_WORN:
				owner1 = OBJ_WORN(cont) ? cont->loc.wearing : cont->loc.carrying;
				// Skip artis on Immortals.
				if (!IS_TRUSTED(owner1))
				{
					found = TRUE;
				}
				break;
			case LOC_ROOM:
				found = TRUE;
				break;
			// Not in a valid location, so skip it.
			case LOC_INSIDE:
			case LOC_NOWHERE:
				break;
			}
			if (found)
			{
				break;
			}
		}
	}
	dummy = NULL;
	// If it's on a PC, and the owner isn't online.
	if (!found && artidata.locType == ARTIFACT_ON_PC && !is_pid_online(artidata.location, TRUE))
	{
		// Try to find it on their pfile.
		dummy = load_dummy_char(get_player_name_from_pid(artidata.location));
		if ((arti1 = get_object_from_char(dummy, vnum1)) == NULL)
		{
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+WCould not find '&+w%s&+W' &+w%d&+W on &+w%s&+W's pfile.&n\n\r",
				 artishort1, vnum1, get_player_name_from_pid(artidata.location));
			nuke_eq(dummy);
			extract_char(dummy);
			return;
		}
	}
	if (!arti1)
	{
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+WStrange, could not find artifact '&+w%s&+W' &+w%d&+W anywhere?!?&n\n\r",
			artishort1, vnum1);
		send_to_char(buf, ch);
		if (dummy)
		{
			nuke_eq(dummy);
			extract_char(dummy);
		}
		return;
	}
	// At this point, we've found arti1 in a valid location (I hope), and have arti2 ready for transfer.
	// Put arti2 in the right spot.
	switch (arti1->loc_p)
	{
	case LOC_CARRIED:
		owner1 = arti1->loc.carrying;
		obj_to_char(arti2, arti1->loc.carrying);
		break;
	case LOC_WORN:
		owner1 = cont->loc.wearing;
		// Find it on their body.
		for (wearloc = 0; wearloc < MAX_WEAR; wearloc++)
		{
			// And move it to their inventory (Also replace with arti2).
			if (owner1->equipment[wearloc] == arti1)
			{
				obj_to_char(unequip_char(owner1, wearloc), owner1);
				equip_char(owner1, arti2, wearloc, TRUE);
			}
		}
		break;
	case LOC_ROOM:
		obj_to_room(arti2, arti1->loc.room);
		break;
	case LOC_INSIDE:
		obj_to_obj(arti2, arti1->loc.inside);
		break;
	// Not in a valid location, so skip it.
	case LOC_NOWHERE:
		snprintf(buf, MAX_STRING_LENGTH,
			 "&+WStrange, artifact '&+w%s&+W' &+w%d&+W has a bad location?!?&n\n\r",
			 artishort1, vnum1);
		send_to_char(buf, ch);
		if (dummy)
		{
			nuke_eq(dummy);
			extract_char(dummy);
		}
		return;
		break;
	}
	// Since arti2 is in position, can pull arti1.
	extract_obj(arti1, TRUE); // Yes, we want to remove arti1 from owned artis.
	// Updata artidata type with arti2 stats.
	// The timer and owned don't change.  Nor does the locType / location since we put it in the same spot arti1 was in.
	artidata.type = IS_IOUN(arti2) ? ARTIFACT_IOUN :
					 (IS_UNIQUE(arti2) ? ARTIFACT_UNIQUE : ARTIFACT_MAJOR);
	// Use the uber-generic update.
	artifact_update_sql(vnum2, artidata.owned, artidata.locType, artidata.location,
			    artidata.timer, artidata.type);
	if (owner1 == dummy)
	{
		owner1 = NULL;
	}
	// Save pfile if applies.
	if (dummy && writeCharacter(dummy, RENT_SWAPARTI, dummy->in_room))
	{
		nuke_eq(dummy);
		extract_char(dummy);
	}
	else if (dummy)
	{
		persistence_alert(AVATAR, "artifact", "offline_swap", "none", "none",
				  "terminal_save_failed", "extract_refused=1");
	}
	// Save in-game owner if applies.
	if (owner1)
	{
		snprintf(buf, MAX_STRING_LENGTH, "&+WYour %s&+W suddenly changes into %s&+W!&n\n\r",
			 artishort1, OBJ_SHORT(arti2));
		send_to_char(buf, owner1);
		snprintf(buf, MAX_STRING_LENGTH, "&+W$n's %s&+W suddenly changes into %s&+W!&n\n\r",
			 artishort1, OBJ_SHORT(arti2));
		act(buf, FALSE, owner1, NULL, 0, TO_ROOM);
		writeCharacter(owner1, RENT_CRASH, owner1->in_room);
	}
	// Save corpse if applies.
	if (artidata.locType == ARTIFACT_ONCORPSE)
	{
		cont = arti2;
		while (OBJ_INSIDE(cont) && cont->loc.inside)
		{
			cont = cont->loc.inside;
			if (cont->type == ITEM_CORPSE &&
			    IS_SET(cont->value[CORPSE_FLAGS], PC_CORPSE))
			{
				writeCorpse(cont);
				break;
			}
		}
	}
	if (OBJ_ROOM(arti2))
	{
		snprintf(buf, MAX_STRING_LENGTH, "&+W%s&+W suddenly changes into %s&+W!&n\n\r",
			 artishort1, OBJ_SHORT(arti2));
		act(buf, FALSE, NULL, arti2, 0, TO_ROOM);
	}

	snprintf(buf, MAX_STRING_LENGTH,
		 "&+WArtifact '&+w%s&+W' &+w%d&+W swapped with artifact '&+w%s&+W' &+w%d&+W.&n\n\r",
		 artishort1, vnum1, OBJ_SHORT(arti2), vnum2);
	send_to_char(buf, ch);
}

// This function walks through the artifact_bind table, gathers its info, then compares
//   it against where the arti is currently.  If it's not currently on the proper char
//   the timer is set to switch owners.  If the timer is up, then the soul switches owners.
void event_artifact_check_bind_sql(P_char /*ch*/, P_char /*vict*/, P_obj /*obj*/, void * /*arg*/)
{
	struct artifact_bind_row
	{
		int vnum;
		int owner_pid;
		long timer;
	};
	static int cursor_vnum = 0;
	artifact_bind_row rows[ARTIFACT_BIND_BATCH_SIZE] = {};
	size_t row_count = 0;
#ifndef __NO_MYSQL__
	MYSQL_RES *res;
	MYSQL_ROW row = NULL;
#endif

	if (!updateArtis)
	{
		cursor_vnum = 0;
		return;
	}

	debug("event_artifact_check_bind_sql(): beginning...");

#ifdef __NO_MYSQL__
	std::vector<flatfile_artifact_record> records;
	std::string error;
	const auto loaded =
		flatfile_artifact_list(persistence_mode_flatfile_root(), &records, &error);
	if (loaded != flatfile_artifact_result::ok)
	{
		logit(LOG_ARTIFACT,
		      "event_artifact_check_bind_sql(): failed to read flat authority: %s",
		      error.empty() ? "unknown error" : error.c_str());
		nevent_periodic_retry_after(ARTIFACT_MAINTENANCE_RETRY_DELAY,
					    "artifact-bind flat read failed");
		return;
	}
	for (const auto &record : records)
	{
		if (record.vnum <= cursor_vnum)
			continue;
		artifact_bind_row &entry = rows[row_count++];
		entry.vnum = record.vnum;
		entry.owner_pid = record.bind_owner_pid;
		entry.timer = record.bind_timer;
		if (row_count == ARTIFACT_BIND_BATCH_SIZE)
			break;
	}
#else
	if (!qry("SELECT vnum, owner_pid, timer FROM artifact_bind WHERE vnum > %d ORDER BY vnum LIMIT %zu",
		 cursor_vnum, ARTIFACT_BIND_BATCH_SIZE))
	{
		debug("event_artifact_check_bind_sql(): Failed initial query.");
		logit(LOG_ARTIFACT,
		      "event_artifact_check_bind_sql(): failed to read from database.");
		nevent_periodic_retry_after(ARTIFACT_MAINTENANCE_RETRY_DELAY,
					    "artifact-bind query failed");
		return;
	}

	res = mysql_store_result(DB);
	if (!res)
	{
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		nevent_periodic_retry_after(ARTIFACT_MAINTENANCE_RETRY_DELAY,
					    "artifact-bind result failed");
		return;
	}

	while (row_count < ARTIFACT_BIND_BATCH_SIZE && (row = mysql_fetch_row(res)))
	{
		artifact_bind_row &entry = rows[row_count++];
		entry.vnum = atoi(row[0]);
		entry.owner_pid = atoi(row[1]);
		entry.timer = row[2] ? atol(row[2]) : 0;
	}
	mysql_free_result(res);
#endif

	if (row_count == 0)
	{
		cursor_vnum = 0;
		debug("event_artifact_check_bind_sql(): no rows after cursor.");
		return;
	}

	const int timer_length = 60 * get_property("artifact.feeding.switch.lootallowance.min", 15);
	const long curr_time = time(NULL);
	int counter = 0;
	for (size_t index = 0; index < row_count; ++index)
	{
		const artifact_bind_row &entry = rows[index];
		arti_data artidata;
		P_char owner;
		P_obj arti = read_object(entry.vnum, VIRTUAL);

		if (get_artifact_data_sql(entry.vnum, &artidata))
		{
			if (artidata.locType == ARTIFACT_ON_PC)
			{
				// If we're on a new owner.
				if (entry.owner_pid != artidata.location)
				{
					// If the timer has expired
					if (entry.timer + timer_length < curr_time)
					{
						// If the owner isn't online, the soul can not merge.
						if ((owner = get_char_online(
							     get_player_name_from_pid(
								     artidata.location))))
						{
							act("&+L$p &+Lmerges with your &+wsoul&+L.",
							    FALSE, owner, arti, 0, TO_CHAR);
							artifact_bind_maintenance_update(
								entry.vnum, artidata.location,
								curr_time);
							logit(LOG_ARTIFACT,
							      "event_artifact_check_bind_sql(): artifact '%s' %d merged with '%s' %d's soul.",
							      arti ? OBJ_SHORT(arti) : "NULL",
							      entry.vnum, J_NAME(owner),
							      artidata.location);
							debug("%3d: '%s&n'%6d merged with '%s' %d's soul.",
							      ++counter,
							      pad_ansi(arti ? OBJ_SHORT(arti) :
									      "NULL",
								       35, TRUE)
								      .c_str(),
							      entry.vnum, J_NAME(owner),
							      artidata.location);
						}
						else
						{
							debug("%3d: '%s&n'%6d is ready, but '%s' %d not online.",
							      ++counter,
							      pad_ansi(arti ? OBJ_SHORT(arti) :
									      "NULL",
								       35, TRUE)
								      .c_str(),
							      entry.vnum,
							      get_player_name_from_pid(
								      artidata.location),
							      artidata.location);
						}
					}
					else if (entry.timer > curr_time)
					{
						debug("%3d: artifact '%s&n'%6d's timer is later than curr_time, owner '%s' %d.",
						      ++counter,
						      pad_ansi(arti ? OBJ_SHORT(arti) : "NULL", 35,
							       TRUE)
							      .c_str(),
						      entry.vnum,
						      get_player_name_from_pid(artidata.location),
						      artidata.location);
						artifact_bind_maintenance_update(
							entry.vnum, artidata.location, curr_time);
					}
				}
			}
			// Display artis that are on the corpse of new owner.
			else if (artidata.locType == ARTIFACT_ONCORPSE &&
				 artidata.location != entry.owner_pid)
			{
				debug("%3d: artifact '%s&n'%6d on corpse of '%s' %d.", ++counter,
				      pad_ansi(arti ? OBJ_SHORT(arti) : "NULL", 35, TRUE).c_str(),
				      entry.vnum, get_player_name_from_pid(artidata.location),
				      artidata.location);
			}
		}
		else if (entry.owner_pid > 0)
		{
			logit(LOG_ARTIFACT,
			      "event_artifact_check_bind_sql(): artifact '%s' %d is unowned, but bound to '%s' %d.",
			      arti ? OBJ_SHORT(arti) : "NULL", entry.vnum,
			      get_player_name_from_pid(entry.owner_pid), entry.owner_pid);
			debug("%3d: artifact '%s&n' %d is unowned, but bound.  Setting owner_pid = -1 and timer = 0.",
			      ++counter,
			      pad_ansi(arti ? OBJ_SHORT(arti) : "NULL", 35, TRUE).c_str(),
			      entry.vnum);
			artifact_bind_maintenance_update(entry.vnum, -1, 0);
		}
		if (arti)
		{
			extract_obj(arti);
		}
	}

	cursor_vnum = rows[row_count - 1].vnum;
	if (row_count == ARTIFACT_BIND_BATCH_SIZE)
	{
		nevent_periodic_continue_after(1);
		return;
	}
	cursor_vnum = 0;
	debug("event_artifact_check_bind_sql(): completed.");
}

// Resets the timers on artifacts that weren't properly bound.
void arti_fixit_sql(P_char ch)
{
#ifdef __NO_MYSQL__
	std::vector<flatfile_artifact_record> records;
	std::string error;
	if (flatfile_artifact_list(persistence_mode_flatfile_root(), &records, &error) !=
	    flatfile_artifact_result::ok)
	{
		logit(LOG_ARTIFACT, "arti_fixit_sql: flat artifact read failed: %s",
		      error.empty() ? "missing or invalid artifact authority" : error.c_str());
		send_to_char("Failed to read artifact data.\n\r", ch);
		return;
	}
	const time_t now = time(NULL);
	const time_t new_time = now + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
	int counter = 0;
	bool found_player_artifact = false;
	for (const auto &record : records)
	{
		if (record.location_type != ARTIFACT_ON_PC)
			continue;
		found_player_artifact = true;
		const auto repaired = flatfile_artifact_repair_player_binding(
			persistence_mode_flatfile_root(), record.vnum, new_time, now, now, &error);
		if (repaired == flatfile_artifact_result::unchanged)
			continue;
		if (repaired != flatfile_artifact_result::ok)
		{
			logit(LOG_ARTIFACT, "arti_fixit_sql: flat repair failed for %d: %s",
			      record.vnum,
			      error.empty() ? "invalid artifact authority" : error.c_str());
			send_to_char_f(ch, "Skipped artifact %d: repair failed.\n\r", record.vnum);
			continue;
		}
		P_obj artifact = read_object(record.vnum, VIRTUAL);
		send_to_char_f(ch, "%3d) '%s&n'%6d - timer reset and now owned by '%s' %d.\n\r",
			       ++counter,
			       pad_ansi(artifact ? OBJ_SHORT(artifact) : "NULL", 35, TRUE).c_str(),
			       record.vnum, get_player_name_from_pid(record.location),
			       record.location);
		if (artifact)
			extract_obj(artifact);
	}
	if (!found_player_artifact)
		send_to_char("Empty set; no artifacts on PC in artifact data.\n\r", ch);
	else if (!counter)
		send_to_char("All artifact bind_data are up to date.\n\r", ch);
#else
	int pid, timer, curr_time;
	int vnum, location, counter;
	time_t new_time;
	P_obj arti;
	MYSQL_RES *res;
	MYSQL_ROW row = NULL;
	struct arti_fix_row
	{
		int vnum;
		int location;
	};
	std::vector<arti_fix_row> rows;

	if (!qry("SELECT vnum, location FROM artifacts WHERE locType=%d", ARTIFACT_ON_PC))
	{
		send_to_char("Failed SELECT command.\n\r", ch);
		return;
	}

	res = mysql_store_result(DB);
	if (!res)
	{
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		return;
	}

	if (mysql_num_rows(res) < 1)
	{
		mysql_free_result(res);
		send_to_char("Empty set; no artifacts on PC in table artifacts.\n\r", ch);
		return;
	}

	while ((row = mysql_fetch_row(res)))
	{
		rows.push_back({ atoi(row[0]), atoi(row[1]) });
	}
	mysql_free_result(res);

	new_time = time(NULL) + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
	curr_time = (int)time(NULL);

	counter = 0;
	// Walk through each arti that's on a PC.
	for (const auto &entry : rows)
	{
		vnum = entry.vnum;
		location = entry.location;
		if (!sql_get_bind_data(vnum, &pid, &timer))
		{
			send_to_char_f(ch, "Skipped artifact %d: bind lookup failed.\n\r", vnum);
			continue;
		}
		timer = curr_time;
		arti = read_object(vnum, VIRTUAL);
		// If the arti is on a different PC, we want to update artifact_bind AND increase the timer to max in artifacts.
		if (location != pid)
		{
			extract_obj(arti);
			sql_update_bind_data(vnum, &location, &timer);
			qry("UPDATE artifacts SET timer = FROM_UNIXTIME(%lu), lastUpdate=SYSDATE() WHERE vnum = %d",
			    new_time, vnum);
			arti_cache_invalidate();
			send_to_char_f(ch,
				       "%3d) '%s&n'%6d - timer reset and now owned by '%s' %d.\n\r",
				       ++counter,
				       pad_ansi(arti ? OBJ_SHORT(arti) : "NULL", 35, TRUE).c_str(),
				       vnum, get_player_name_from_pid(location), location);
		}
		extract_obj(arti);
	}
	if (counter == 0)
	{
		send_to_char("All artifact bind_data are up to date.\n\r", ch);
	}
#endif
}

// syncs all in-game artifact locations to the database
void arti_sync_sql(P_char ch)
{
	P_obj obj;
	int counter = 0;

	send_to_char("Syncing all in-game artifacts to database...\n\r", ch);

	// iterate through all objects in game
	for (obj = object_list; obj; obj = obj->next)
	{
		if (!IS_ARTIFACT(obj))
			continue;

		// update this artifact's location in DB
		artifact_update_location_sql(obj);
		counter++;
	}

	arti_cache_invalidate();
	send_to_char_f(ch, "Synced %d artifacts to database.\n\r", counter);
}

// syncs artifact ownership from player_items table to artifacts_mortal
void arti_syncdb_sql(P_char ch)
{
#ifdef __NO_MYSQL__
	std::vector<flatfile_item_ownership_record> owned_items;
	std::string error;
	if (flatfile_item_repository_list_active_player_items(persistence_mode_flatfile_root(),
							      &owned_items, &error) !=
	    flatfile_item_repository_result::ok)
	{
		logit(LOG_ARTIFACT, "arti_syncdb_sql: flat item authority read failed: %s",
		      error.empty() ? "missing or invalid item authority" : error.c_str());
		send_to_char("Failed to read saved item ownership.\n\r", ch);
		return;
	}
	std::vector<flatfile_artifact_player_item> player_items;
	try
	{
		player_items.reserve(owned_items.size());
		for (const auto &item : owned_items)
		{
			if (item.owner.id > static_cast<uint64_t>(INT32_MAX))
			{
				send_to_char(
					"Saved item ownership contains an invalid player id.\n\r",
					ch);
				return;
			}
			player_items.push_back({ item.vnum, static_cast<int32_t>(item.owner.id) });
		}
	}
	catch (const std::bad_alloc &)
	{
		send_to_char("Not enough memory to reconcile artifact ownership.\n\r", ch);
		return;
	}
	flatfile_artifact_reconcile_result counts;
	const auto reconciled = flatfile_artifact_reconcile_players(
		persistence_mode_flatfile_root(), player_items, time(NULL), &counts, &error);
	if (reconciled != flatfile_artifact_result::ok &&
	    reconciled != flatfile_artifact_result::unchanged)
	{
		logit(LOG_ARTIFACT, "arti_syncdb_sql: flat artifact reconciliation failed: %s",
		      error.empty() ? "invalid or conflicting authority" : error.c_str());
		send_to_char("Failed to reconcile artifact ownership.\n\r", ch);
		return;
	}
	arti_cache_invalidate();
	send_to_char_f(ch,
		       "Cleared %zu, updated %zu artifact ownerships from flat player saves.\n\r",
		       counts.cleared, counts.updated);
#else
	extern MYSQL *DB;
	if (!DB)
	{
		send_to_char("Database not connected.\n\r", ch);
		return;
	}

	send_to_char("Syncing artifact ownership from player saves...\n\r", ch);

	// clear artifacts table (main table used by god view)
	const char *clear_artifacts_sql =
		"UPDATE artifacts SET location = 0, owned = 'N', locType = 1, lastUpdate = SYSDATE() "
		"WHERE locType = 3 OR locType = 5";
	if (!sql_trace_exec("arti_fixit/clear_artifacts", clear_artifacts_sql,
			    strlen(clear_artifacts_sql), true, false))
	{
		send_to_char_f(ch, "Error clearing artifacts: %s\n\r", mysql_error(DB));
		return;
	}
	int cleared = mysql_affected_rows(DB);

	// clear artifacts_mortal table
	const char *clear_mortal_sql =
		"UPDATE artifacts_mortal SET location = 0, owned = 'N', locType = 1";
	sql_trace_exec("arti_fixit/clear_mortal", clear_mortal_sql, strlen(clear_mortal_sql), true,
		       false);

	// clear artifact_bind owner_pid for pc-held artifacts
	const char *clear_bind_sql = "UPDATE artifact_bind SET owner_pid = -1, timer = 0";
	sql_trace_exec("arti_fixit/clear_bind", clear_bind_sql, strlen(clear_bind_sql), true,
		       false);

	// update artifacts table from player_items
	const char *sync_artifacts_sql =
		"UPDATE artifacts a "
		"JOIN player_items pi ON pi.vnum = a.vnum "
		"JOIN player_data pd ON pd.pid = pi.pid AND pd.active = 1 "
		"SET a.location = pi.pid, a.owned = 'Y', a.locType = 3, a.lastUpdate = SYSDATE()";
	if (!sql_trace_exec("arti_fixit/sync_artifacts", sync_artifacts_sql,
			    strlen(sync_artifacts_sql), true, false))
	{
		send_to_char_f(ch, "Error syncing artifacts: %s\n\r", mysql_error(DB));
		return;
	}
	int updated = mysql_affected_rows(DB);

	// update artifacts_mortal table from player_items
	const char *sync_mortal_sql = "UPDATE artifacts_mortal am "
				      "JOIN player_items pi ON pi.vnum = am.vnum "
				      "JOIN player_data pd ON pd.pid = pi.pid AND pd.active = 1 "
				      "SET am.location = pi.pid, am.owned = 'Y', am.locType = 3";
	sql_trace_exec("arti_fixit/sync_mortal", sync_mortal_sql, strlen(sync_mortal_sql), true,
		       false);

	// update artifact_bind with owner_pid from player_items
	const char *sync_bind_sql = "UPDATE artifact_bind ab "
				    "JOIN player_items pi ON pi.vnum = ab.vnum "
				    "JOIN player_data pd ON pd.pid = pi.pid AND pd.active = 1 "
				    "SET ab.owner_pid = pi.pid, ab.timer = UNIX_TIMESTAMP()";
	sql_trace_exec("arti_fixit/sync_bind", sync_bind_sql, strlen(sync_bind_sql), true, false);

	arti_cache_invalidate();
	send_to_char_f(ch, "Cleared %d, updated %d artifact ownerships from player saves.\n\r",
		       cleared, updated);
#endif
}

// Resets the 'soul' of the artifact of vnum == arg.
// It resets the timer to 0 also, so it will merge asap.
void arti_reset_sql(P_char ch, char *arg)
{
	int vnum;

	if (!*arg || !strcmp(arg, "?") || !strcmp(arg, "help"))
	{
		send_to_char(
			"This command resets artifact bind data (The &+Lsoul&n of the artifact).\n\r",
			ch);
		send_to_char("It should only be used by those who know what they're doing.\n\r",
			     ch);
		send_to_char(
			"This will unmerge the soul from the owner of the artifact, but set the timer to 0,\n\r",
			ch);
		send_to_char(
			"  so it will merge as soon as the owner triggers artifact_switch_check.\n\r",
			ch);
		return;
	}
	if (GET_LEVEL(ch) < FORGER)
	{
		send_to_char("Maybe you could ask someone of higher level to do this.\n\r", ch);
		return;
	}

	if (!strcmp("fixit", arg))
	{
		arti_fixit_sql(ch);
		return;
	}

	if (!strcmp("sync", arg))
	{
		arti_sync_sql(ch);
		return;
	}

	if (!strcmp("syncdb", arg))
	{
		arti_syncdb_sql(ch);
		return;
	}

	if (isname("all", arg))
	{
		if (GET_LEVEL(ch) < OVERLORD)
		{
			send_to_char("Maybe you could ask someone of higher level to do this.\n\r",
				     ch);
			return;
		}
		if (!isname("confirm", arg))
		{
			send_to_char(
				"&+RThis will reset all the artifacts' souls!  If you want to do this, it requires &+wconfirmation&+R.&n\n\r",
				ch);
			send_to_char("&=LRThis is probably a really bad idea!&n\n\r", ch);
			return;
		}
		vnum = -1;
	}
	else if ((vnum = atoi(arg)) <= 0)
	{
		send_to_char("This command requires a vnum for an argument.\n\r", ch);
		return;
	}
	if (vnum > 0)
	{
#ifdef __NO_MYSQL__
		std::string error;
		const auto reset = flatfile_artifact_bind_update(persistence_mode_flatfile_root(),
								 vnum, -1, 0, &error);
		if (reset == flatfile_artifact_result::ok ||
		    reset == flatfile_artifact_result::unchanged)
#else
		if (qry("UPDATE artifact_bind SET owner_pid = -1, timer = 0 WHERE vnum = %d", vnum))
#endif
		{
			send_to_char_f(ch, "Artifact vnum %d has a hungry soul.\n\r", vnum);
		}
		else
		{
#ifdef __NO_MYSQL__
			logit(LOG_ARTIFACT, "arti_reset_sql: flat binding reset failed for %d: %s",
			      vnum,
			      error.empty() ? "missing or invalid artifact authority" :
					      error.c_str());
#endif
			send_to_char("Update operation failed.\n\r", ch);
		}
	}
	else
	{
#ifdef __NO_MYSQL__
		std::string error;
		const auto reset =
			flatfile_artifact_bind_reset_all(persistence_mode_flatfile_root(), &error);
		if (reset == flatfile_artifact_result::ok ||
		    reset == flatfile_artifact_result::unchanged)
#else
		if (qry("UPDATE artifact_bind SET owner_pid = -1, timer = 0"))
#endif
		{
			send_to_char("All artifacts' souls are hungry for an owner now.\n\r", ch);
		}
		else
		{
#ifdef __NO_MYSQL__
			logit(LOG_ARTIFACT, "arti_reset_sql: flat binding reset-all failed: %s",
			      error.empty() ? "missing or invalid artifact authority" :
					      error.c_str());
#endif
			send_to_char("Update operation failed.\n\r", ch);
		}
	}
}

// Returns the first mob in the game with said vnum.
P_char find_mob_in_game(int vnum)
{
	P_char mob;

	for (mob = character_list; mob; mob = mob->next)
	{
		if (IS_NPC(mob) && GET_VNUM(mob) == vnum)
		{
			return mob;
		}
	}

	return NULL;
}

// Loads the artis that were on a random mob and owned back into the boot.
void addOnMobArtis_sql()
{
	P_obj arti;
	P_char mob;
#ifndef __NO_MYSQL__
	MYSQL_RES *res;
	MYSQL_ROW row;
#endif

	logit(LOG_ARTIFACT, "addOnMobArtis_sql: Beginning.");

#ifdef __NO_MYSQL__
	std::vector<flatfile_artifact_record> records;
	std::string error;
	if (flatfile_artifact_list(persistence_mode_flatfile_root(), &records, &error) !=
	    flatfile_artifact_result::ok)
	{
		logit(LOG_ARTIFACT, "addOnMobArtis_sql: flat artifact read failed: %s",
		      error.empty() ? "missing or invalid artifact authority" : error.c_str());
		return;
	}
	bool found = false;
	for (const auto &record : records)
	{
		if (!record.owned || record.location_type != ARTIFACT_ON_NPC)
			continue;
		found = true;
		if (!(arti = read_object(record.vnum, VIRTUAL)))
		{
			logit(LOG_ARTIFACT, "addOnMobArtis_sql: Could not load object vnum %d.",
			      record.vnum);
			continue;
		}
		if (!(mob = find_mob_in_game(record.location)))
		{
			logit(LOG_ARTIFACT, "addOnMobArtis_sql: Could not find mob vnum %d.",
			      record.location);
			extract_obj(arti);
			continue;
		}
		obj_to_char(arti, mob);
	}
	if (!found)
		logit(LOG_ARTIFACT, "addOnMobArtis_sql: No owned artifacts found on NPCs.");
#else
	qry("SELECT vnum, location FROM artifacts WHERE owned='Y' AND locType=%d", ARTIFACT_ON_NPC);

	if ((res = mysql_store_result(DB)) != NULL)
	{
		if (mysql_num_rows(res) < 1)
		{
			logit(LOG_ARTIFACT, "addOnMobArtis_sql: No owned artifacts found on NPCs.");
		}
		else
		{
			while ((row = mysql_fetch_row(res)))
			{
				if (!(arti = read_object(atoi(row[0]), VIRTUAL)))
				{
					logit(LOG_ARTIFACT,
					      "addOnMobArtis_sql: Could not load object vnum %d.",
					      atoi(row[0]));
					continue;
				}
				if (!(mob = find_mob_in_game(atoi(row[1]))))
				{
					logit(LOG_ARTIFACT,
					      "addOnMobArtis_sql: Could not find mob vnum %d.",
					      atoi(row[1]));
					extract_obj(arti);
					continue;
				}
				obj_to_char(arti, mob);
			}
		}
		mysql_free_result(res);
	}
	else
	{
		logit(LOG_ARTIFACT, "addOnMobArtis_sql: Could not pull on mob arti list.");
	}
#endif

	logit(LOG_ARTIFACT, "addOnMobArtis_sql: Ending.");
}

void arti_player_sql(P_char ch, char *arg)
{
#ifndef __NO_MYSQL__
	char buf[MAX_STRING_LENGTH], locationBuf[MAX_STRING_LENGTH], timeBuf[128], *name;
	int pid, vnum, locType, minutes, hours;
	long totalTime;
	bool shownData, negTime;
	P_obj arti;
	MYSQL_RES *res;
	MYSQL_ROW row;

	if ((pid = atoi(arg)) < 1)
	{
		if ((pid = get_player_pid_from_name(arg)) < 1)
		{
			send_to_char(
				"The '&+wartifact player&n' command requires a valid player name or pid.\n\r",
				ch);
			return;
		}
	}
	if ((name = get_player_name_from_pid(pid)) == NULL)
	{
		snprintf(buf, MAX_STRING_LENGTH,
			 "'%s' was not found to be a valid player name or pid.\n", arg);
		send_to_char(buf, ch);
		return;
	}

	snprintf(buf, MAX_STRING_LENGTH,
		 "&+YOwner                  Time      Last Update           Artifact\r\n\r\n");
	send_to_char(buf, ch);

	if (!qry("SELECT vnum, locType, location, owned, UNIX_TIMESTAMP(timer), lastUpdate FROM artifacts WHERE location=%d",
		 pid))
	{
		send_to_char("&+RError with query attempt.  Aborting...\n", ch);
		return;
	}

	if (!(res = mysql_store_result(DB)))
	{
		send_to_char("&+RError storing query result.  Aborting...\n", ch);
		return;
	}

	if (mysql_num_rows(res) < 1)
	{
		mysql_free_result(res);
		send_to_char("No artifacts found.\n\r", ch);
		return;
	}

	shownData = FALSE;
	while ((row = mysql_fetch_row(res)))
	{
		vnum = atoi(row[0]);
		locType = atoi(row[1]);

		// In case there's on in the room with vnum == pid or such.
		if (locType != ARTIFACT_ON_PC && locType != ARTIFACT_ONCORPSE)
		{
			continue;
		}

		// Tryin' load a copy of the arti for display purposes.
		arti = read_object(vnum, VIRTUAL);
		if (!arti || !IS_ARTIFACT(arti))
		{
			debug("list_artifacts_sql: Non artifact on arti list: '%s' %d.",
			      (arti == NULL) ? "NULL" : arti->short_description, vnum);
			// Pull arti if it loaded.
			if (arti)
			{
				extract_obj(arti);
			}
			continue;
		}

		if (locType == ARTIFACT_ON_PC)
		{
			snprintf(locationBuf, MAX_STRING_LENGTH, "%-21s", name);
		}
		else if (locType == ARTIFACT_ONCORPSE)
		{
			snprintf(buf, MAX_STRING_LENGTH, "%s's corpse", name);
			checked_snprintf(locationBuf, MAX_STRING_LENGTH, "%-21s", buf);
		}
		else
		{
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+RError reading query result.  Skipping... '%s' %d.\n",
				 OBJ_SHORT(arti), OBJ_VNUM(arti));
			send_to_char(buf, ch);
			extract_obj(arti);
			continue;
		}

		negTime = FALSE;
		// totalTime (left to poof in sec) is the timer (time at which it poofs) - now.
		if (atol(row[4]) == 0)
		{
			totalTime = 0;
		}
		if ((totalTime = atol(row[4]) - time(NULL)) < 0)
		{
			negTime = TRUE;
			totalTime *= -1;
		}
		// Convert to minutes.
		totalTime /= 60;
		minutes = totalTime % 60;
		// Convert to hours.
		totalTime /= 60;
		hours = totalTime % 24;

		snprintf(timeBuf, sizeof timeBuf, "%c%2ld:%02d:%02d", negTime ? '-' : ' ',
			 totalTime / 24, hours, minutes);

		checked_snprintf(buf, MAX_STRING_LENGTH, "%s&n%-11s %-22s%s (#%d)\r\n", locationBuf,
				 timeBuf, row[5], OBJ_SHORT(arti), vnum);
		send_to_char(buf, ch);
		shownData = TRUE;
		extract_obj(arti, FALSE);
	}
	mysql_free_result(res);

	if (!shownData)
		send_to_char("No artifacts found.\n\r", ch);
#else
	char buf[MAX_STRING_LENGTH], location_buffer[MAX_STRING_LENGTH], time_buffer[128];
	int pid = atoi(arg);
	if (pid < 1)
	{
		pid = get_player_pid_from_name(arg);
		if (pid < 1)
		{
			send_to_char(
				"The '&+wartifact player&n' command requires a valid player name or pid.\n\r",
				ch);
			return;
		}
	}
	char *name = get_player_name_from_pid(pid);
	if (!name)
	{
		snprintf(buf, sizeof(buf), "'%s' was not found to be a valid player name or pid.\n",
			 arg);
		send_to_char(buf, ch);
		return;
	}
	std::vector<flatfile_artifact_record> records;
	std::string error;
	if (flatfile_artifact_list(persistence_mode_flatfile_root(), &records, &error) !=
	    flatfile_artifact_result::ok)
	{
		logit(LOG_ARTIFACT, "arti_player_sql: flat artifact read failed: %s",
		      error.empty() ? "missing or invalid artifact authority" : error.c_str());
		send_to_char("Artifact data is unavailable.\n\r", ch);
		return;
	}
	snprintf(buf, sizeof(buf),
		 "&+YOwner                  Time      Last Update           Artifact\r\n\r\n");
	send_to_char(buf, ch);
	bool shown_data = false;
	for (const auto &record : records)
	{
		if (record.location != pid || (record.location_type != ARTIFACT_ON_PC &&
					       record.location_type != ARTIFACT_ONCORPSE))
			continue;
		P_obj artifact = read_object(record.vnum, VIRTUAL);
		if (!artifact || !IS_ARTIFACT(artifact))
		{
			debug("arti_player_sql: Non artifact on arti list: '%s' %d.",
			      artifact ? artifact->short_description : "NULL", record.vnum);
			if (artifact)
				extract_obj(artifact, FALSE);
			continue;
		}
		if (record.location_type == ARTIFACT_ON_PC)
			snprintf(location_buffer, sizeof(location_buffer), "%-21s", name);
		else
		{
			snprintf(buf, sizeof(buf), "%s's corpse", name);
			checked_snprintf(location_buffer, sizeof(location_buffer), "%-21s", buf);
		}
		long total_time = static_cast<long>(record.timer - time(NULL));
		bool negative_time = total_time < 0;
		if (negative_time)
			total_time *= -1;
		total_time /= 60;
		const int minutes = total_time % 60;
		total_time /= 60;
		const int hours = total_time % 24;
		const long days = record.timer ? total_time / 24 : 0;
		snprintf(time_buffer, sizeof(time_buffer), "%c%2ld:%02d:%02d",
			 negative_time && record.timer ? '-' : ' ', days, record.timer ? hours : 0,
			 record.timer ? minutes : 0);
		char update_buffer[32] = "";
		const time_t updated = static_cast<time_t>(record.last_update);
		struct tm update_time;
		if (record.last_update > 0 && localtime_r(&updated, &update_time))
			strftime(update_buffer, sizeof(update_buffer), "%Y-%m-%d %H:%M:%S",
				 &update_time);
		checked_snprintf(buf, sizeof(buf), "%s&n%-11s %-22s%s (#%d)\r\n", location_buffer,
				 time_buffer, update_buffer, artifact->short_description,
				 record.vnum);
		send_to_char(buf, ch);
		shown_data = true;
		extract_obj(artifact, FALSE);
	}
	if (!shown_data)
		send_to_char("No artifacts found.\n\r", ch);
#endif
}
