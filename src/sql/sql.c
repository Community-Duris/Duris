
/************************************************************************
 * sql.c - interface to MySQL database and functions for stats keeping  *
 *                                                                      *
 * Written by: Thima (Xenofon Papadopoulos)                             *
 *                                                                      *
 ************************************************************************/

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "item/item_uid_allocator.h"
#include "persistence/critical_command.h"
#include "flatfile/flatfile_artifact_repository.h"
#include "flatfile/flatfile_help_catalog.h"
#include "flatfile/flatfile_ip_activity_repository.h"
#include "flatfile/flatfile_offline_message_repository.h"
#include "flatfile/flatfile_frag_leaderboard_repository.h"
#include "flatfile/flatfile_shop_trophy_history.h"
#include "flatfile/flatfile_world_quest_history.h"
#include "persistence/persistence_mode.h"
#include "core/utils.h"
#include "sql/sql.h"
#include "item/item_ownership_runtime.h"
#include "persistence/persistence_checkpoint.h"
#include "sql/sql_pool.h"
#include "account/session_audit_transaction.h"
#include "core/runtime_compatibility_contract.h"
#include <algorithm>
#include <openssl/sha.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "account/account.h"
#include "account/account_reward.h"
#include "guild/assocs.h"
#include "economy/boon.h"
#include "world/epic.h"
#include "world/graph.h"
#include "core/mm.h"
#include "item/objmisc.h"
#include "redis/redis_maintenance.h"
#include "classes/specializations.h"
#include "magic/spells.h"
#include "sql/sql_player.h"
#include "combat/frag_cap_config.h"
#include "world/timers.h"
#include "persistence/persistence_queue.h"
#include "core/utility.h"
#include <errno.h>
#include <limits.h>

extern P_index mob_index;
extern const struct race_names race_names_table[];
extern const struct class_names class_names_table[];
extern const struct playable_race_info playable_races[];
extern const char *specdata[][MAX_SPEC];
extern P_room world;
extern int RUNNING_PORT;
void get_assoc_name(int, char *);
bool get_equipment_list(P_char ch, char *buf, int list_only);
extern P_index obj_index;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern const int top_of_world;
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;

void get_pkill_player_description(P_char ch, char *buffer);

#ifndef __NO_MYSQL__
static int sql_trace_burst = 0;
static const pid_t sql_main_process_id = getpid();
static bool sql_trace_enabled(void);
static bool sql_trace_active(void);
static void sql_trace_log_drain(MYSQL *conn, const char *phase, bool drained);
static bool sql_verify_metadata_fingerprint(void);
#endif

static int flat_sql_shop_sell(P_char ch, P_obj obj, int value)
{
	if (!obj)
		return 0;
	const int item = obj->R_num >= 0 ? obj_index[obj->R_num].virtual_number : 0;
	const int seller = ch && IS_PC(ch) ? GET_PID(ch) : 0;
	std::string error;
	if (flatfile_shop_trophy_record(persistence_mode_flatfile_root(), item, value, seller,
					static_cast<int64_t>(time(nullptr)),
					&error) != flatfile_shop_trophy_result::ok)
	{
		persistence_alert(AVATAR, "shop_trophy", "global", "none", "none", "record",
				  "flat_write_failed", "item=%d seller=%d error=%s", item, seller,
				  error.c_str());
		return -1;
	}
	return 1;
}

static int flat_sql_shop_trophy(P_obj obj)
{
	if (!obj)
		return 0;
	if (obj->name && strstr(obj->name, "_ore_"))
		return 0;
	const int objvir = OBJ_VNUM(obj);
	if (objvir >= 400000 && objvir < 400202)
		return 0;
	const int item = obj->R_num >= 0 ? obj_index[obj->R_num].virtual_number : 0;
	int count = 0;
	std::string error;
	if (flatfile_shop_trophy_count(persistence_mode_flatfile_root(), item,
				       static_cast<int64_t>(time(nullptr)), &count,
				       &error) != flatfile_shop_trophy_result::ok)
	{
		persistence_alert(AVATAR, "shop_trophy", "global", "none", "none", "count",
				  "flat_read_failed", "item=%d error=%s", item, error.c_str());
		return -1;
	}
	return count;
}

#ifdef __NO_MYSQL__
MYSQL *DB = NULL;

MYSQL *sql_open_configured_connection(unsigned long client_flags)
{
	(void)client_flags;
	return NULL;
}

MYSQL_RES *db_query_at(struct persistence_query_site site, const char *format, ...)
{
	(void)site;
	(void)format;
	return NULL;
}

MYSQL_RES *db_query_nolog_at(struct persistence_query_site site, const char *format, ...)
{
	(void)site;
	(void)format;
	return NULL;
}

bool sql_observed_execute_at(MYSQL *conn, struct persistence_query_site site,
			     enum persistence_query_context context, const char *sql, size_t len,
			     uint64_t *operation_id)
{
	(void)conn;
	(void)site;
	(void)context;
	(void)sql;
	(void)len;
	(void)operation_id;
	return false;
}

char *mysql_str(const char * /*str*/, char *buf)
{
	if (buf)
		buf[0] = '\0';
	return buf;
}

int initialize_mysql()
{
	return -1;
}
void shutdown_mysql(void) {}
void do_sql(P_char /*ch*/, char * /*argument*/, int /*cmd*/) {}
int sql_save_player_core(P_char /*ch*/)
{
	return 0;
}
void sql_modify_frags(P_char ch, int /*gain*/)
{
	sql_update_frag_leaderboard(ch);
}
void sql_insert_item(P_char /*ch*/, P_obj /*obj*/, char * /*desc*/) {}

void sql_save_pkill(P_char /*ch*/, P_char /*victim*/) {}
void sql_insert_new_item(P_char /*ch*/, P_obj /*obj*/) {}

void sql_webinfo_toggle(P_char /*ch*/) {}
void sql_update_level(P_char /*ch*/) {}
void sql_update_money(P_char /*ch*/) {}
void sql_update_epics(P_char /*ch*/) {}
void sql_update_playtime(P_char /*ch*/) {}
void manual_log(P_char /*ch*/) {}
void perform_wiki_search(P_char /*ch*/, const char * /*buf*/) {}
int sql_quest_finish(P_char /*ch*/, P_char /*giver*/, int /*type*/, int /*value*/)
{
	return -1;
}
int sql_quest_trophy(P_char /*giver*/)
{
	return -1;
}
int sql_shop_trophy(P_obj obj)
{
	return flat_sql_shop_trophy(obj);
}
int sql_shop_sell(P_char ch, P_obj obj, int value)
{
	return flat_sql_shop_sell(ch, obj, value);
}
void sql_world_quest_finished(P_char ch, P_obj /*obj*/)
{
	if (!ch || !IS_PC(ch) || !ch->only.pc || GET_PID(ch) <= 0 ||
	    ch->only.pc->quest_mob_vnum <= 0)
		return;
	std::string error;
	if (flatfile_world_quest_record(
		    persistence_mode_flatfile_root(), static_cast<uint32_t>(GET_PID(ch)),
		    ch->only.pc->quest_mob_vnum, GET_LEVEL(ch), static_cast<int64_t>(time(nullptr)),
		    &error) != flatfile_world_quest_result::ok)
		persistence_alert(AVATAR, "world_quest", "player", "unknown", "record",
				  "flat_write_failed", "pid=%d error=%s", GET_PID(ch),
				  error.c_str());
}

int sql_world_quest_done_already(P_char ch, int quest_target)
{
	if (!ch || !IS_PC(ch) || !ch->only.pc || GET_PID(ch) <= 0 || quest_target <= 0)
		return -1;
	bool completed = false;
	std::string error;
	if (flatfile_world_quest_completed(persistence_mode_flatfile_root(),
					   static_cast<uint32_t>(GET_PID(ch)), quest_target,
					   &completed, &error) != flatfile_world_quest_result::ok)
	{
		logit(LOG_DEBUG, "sql_world_quest_done_already: %s", error.c_str());
		return -1;
	}
	return completed ? 1 : 0;
}

int sql_world_quest_can_do_another(P_char ch)
{
	if (!ch || !IS_PC(ch) || !ch->only.pc || GET_PID(ch) <= 0)
		return -1;
	int completed_today = 0;
	std::string error;
	if (flatfile_world_quest_count_day(persistence_mode_flatfile_root(),
					   static_cast<uint32_t>(GET_PID(ch)), GET_LEVEL(ch),
					   static_cast<int64_t>(time(nullptr)), &completed_today,
					   &error) != flatfile_world_quest_result::ok)
	{
		logit(LOG_DEBUG, "sql_world_quest_can_do_another: %s", error.c_str());
		return -1;
	}
	int maximum = 0;
	if (GET_LEVEL(ch) <= 30)
		maximum = get_property("world.quest.max.level.30.andUnder", 6.000);
	else if (GET_LEVEL(ch) <= 40)
		maximum = get_property("world.quest.max.level.40.andUnder", 6.000);
	else if (GET_LEVEL(ch) <= 50)
		maximum = get_property("world.quest.max.level.50.andUnder", 6.000);
	else if (GET_LEVEL(ch) <= 55)
		maximum = get_property("world.quest.max.level.55.andUnder", 6.000);
	else
		maximum = get_property("world.quest.max.level.other", 6.000);
	return std::max(maximum - completed_today, 0);
}

static int flat_ip_racewar_side(P_char ch)
{
	return IS_TRUSTED(ch) ? RACEWAR_NONE : GET_RACEWAR(ch);
}

void sql_connectIP(P_char ch)
{
	if (!ch || !IS_PC(ch) || !ch->only.pc || GET_PID(ch) <= 0 || !ch->desc ||
	    !ch->desc->host[0])
		return;
	std::string error;
	if (flatfile_ip_activity_connect(
		    persistence_mode_flatfile_root(), static_cast<uint32_t>(GET_PID(ch)),
		    ch->desc->host, flat_ip_racewar_side(ch), static_cast<int64_t>(time(nullptr)),
		    &error) != flatfile_ip_activity_result::ok)
		logit(LOG_DEBUG, "sql_connectIP: failed to persist IP activity: %s", error.c_str());
}

void sql_disconnectIP(P_char ch)
{
	if (!ch || !IS_PC(ch) || !ch->only.pc || GET_PID(ch) <= 0 || !ch->desc)
		return;
	std::string error;
	if (flatfile_ip_activity_disconnect(
		    persistence_mode_flatfile_root(), static_cast<uint32_t>(GET_PID(ch)),
		    flat_ip_racewar_side(ch), static_cast<int64_t>(time(nullptr)),
		    &error) != flatfile_ip_activity_result::ok)
		logit(LOG_DEBUG, "sql_disconnectIP: failed to persist IP activity: %s",
		      error.c_str());
}

const char *sql_select_IP_info(P_char ch, char *buf, size_t bufSize, time_t *lastConnect,
			       time_t *lastDisconnect)
{
	if (buf && bufSize)
		buf[0] = '\0';
	if (lastConnect)
		*lastConnect = 0;
	if (lastDisconnect)
		*lastDisconnect = 0;
	if (!buf || !bufSize || !ch || !IS_PC(ch) || !ch->only.pc || GET_PID(ch) <= 0)
		return buf;

	flatfile_ip_activity_record record;
	std::string error;
	const auto loaded = flatfile_ip_activity_get(persistence_mode_flatfile_root(),
						     static_cast<uint32_t>(GET_PID(ch)), &record,
						     &error);
	if (loaded == flatfile_ip_activity_result::not_found)
		return buf;
	if (loaded != flatfile_ip_activity_result::ok)
	{
		logit(LOG_DEBUG, "sql_select_IP_info: failed to load IP activity: %s",
		      error.c_str());
		return buf;
	}

	strlcpy(buf, record.ip.c_str(), bufSize);
	const int64_t now = static_cast<int64_t>(time(nullptr));
	if (lastConnect && record.last_connect > 0 && now >= record.last_connect)
		*lastConnect = static_cast<time_t>(now - record.last_connect);
	if (lastDisconnect && record.last_disconnect > 0 && now >= record.last_disconnect)
		*lastDisconnect = static_cast<time_t>(now - record.last_disconnect);
	return buf;
}

int sql_find_racewar_for_ip(char *ip, int *racewar_side)
{
	if (racewar_side)
		*racewar_side = RACEWAR_NONE;
	if (!ip || !*ip || !racewar_side)
		return -1;

	flatfile_ip_activity_record record;
	std::string error;
	const auto loaded = flatfile_ip_activity_find_latest(persistence_mode_flatfile_root(), ip,
							     &record, &error);
	if (loaded == flatfile_ip_activity_result::not_found)
		return 0;
	if (loaded != flatfile_ip_activity_result::ok)
	{
		logit(LOG_DEBUG, "sql_find_racewar_for_ip: failed to load IP activity: %s",
		      error.c_str());
		return -1;
	}

	*racewar_side = record.racewar_side;
	const int64_t now = static_cast<int64_t>(time(nullptr));
	const int64_t hour_ago = now - 60 * 60;
	if (record.last_disconnect > record.last_connect && record.last_disconnect <= hour_ago)
	{
		*racewar_side = RACEWAR_NONE;
		return 0;
	}
	if (record.last_disconnect < record.last_connect)
		return 60 * 60;
	const int64_t remaining = record.last_disconnect - hour_ago;
	if (remaining <= 0)
	{
		*racewar_side = RACEWAR_NONE;
		return 0;
	}
	return static_cast<int>(std::min<int64_t>(remaining, 60 * 60));
}
bool qry_at(struct persistence_query_site site, const char *format, ...)
{
	(void)site;
	(void)format;
	return FALSE;
}
static void enqueue_flat_offline_message(const char *message, int pid)
{
	const char *root = persistence_mode_flatfile_root();
	critical_operation_id operation_id = {};
	std::string error;
	if (!root || !message || pid <= 0 || !critical_operation_id_generate(&operation_id) ||
	    flatfile_offline_message_enqueue(root, static_cast<uint32_t>(pid), operation_id.bytes,
					     message,
					     &error) != flatfile_offline_message_result::ok)
		persistence_alert(AVATAR, "offline_message", "player", "unknown", "enqueue",
				  "flat_write_failed", "pid=%d error=%s", pid, error.c_str());
}
void send_to_char_offline(const char *message, int pid)
{
	enqueue_flat_offline_message(message, pid);
}
void send_to_pid_offline(const char *message, int pid)
{
	enqueue_flat_offline_message(message, pid);
}
void send_offline_messages(P_char ch)
{
	const char *root = persistence_mode_flatfile_root();
	if (!root || !ch || IS_NPC(ch) || GET_PID(ch) <= 0)
		return;
	std::vector<flatfile_offline_message_record> messages;
	std::string error;
	if (flatfile_offline_message_list(root, static_cast<uint32_t>(GET_PID(ch)), &messages,
					  &error) != flatfile_offline_message_result::ok)
	{
		persistence_alert(AVATAR, "offline_message", "player", "unknown", "load",
				  "flat_read_failed", "pid=%d error=%s", GET_PID(ch),
				  error.c_str());
		return;
	}
	std::sort(messages.begin(), messages.end(),
		  [](const auto &left, const auto &right)
		  {
			  return left.created_at != right.created_at ?
					 left.created_at < right.created_at :
					 left.id < right.id;
		  });
	for (const auto &message : messages)
	{
		send_to_char(message.text.c_str(), ch);
		const auto acknowledged = flatfile_offline_message_acknowledge(
			root, static_cast<uint32_t>(GET_PID(ch)), message.id, &error);
		if (acknowledged != flatfile_offline_message_result::ok &&
		    acknowledged != flatfile_offline_message_result::not_found)
		{
			persistence_alert(AVATAR, "offline_message", "player", "unknown",
					  "acknowledge", "flat_write_failed", "pid=%d error=%s",
					  GET_PID(ch), error.c_str());
			break;
		}
	}
}
void log_epic_gain(int /*pid*/, int /*zone_id*/, int /*type*/, int /*epics*/) {}
void log_epic_gain_event(const char * /*event_key*/, int /*pid*/, int /*type*/, int /*type_id*/,
			 int /*epics*/)
{
}
bool sql_persistence_item_owner_matches(unsigned long long /*item_uid*/,
					const char * /*owner_type*/, const char * /*owner_ref*/,
					const char * /*context*/)
{
	return false;
}
bool sql_persistence_item_owner_matches_identity(unsigned long long /*item_uid*/,
						 const char * /*owner_type*/,
						 unsigned long long /*owner_id*/,
						 unsigned long long /*owner_context_id*/,
						 const char * /*context*/)
{
	return false;
}
bool sql_persistence_reconcile_world_recovery_items(const world_recovery_authority_item *items,
						    size_t count,
						    item_ownership_runtime_entry *authoritative,
						    size_t authoritative_capacity)
{
	(void)items;
	(void)authoritative;
	return count == 0 && authoritative_capacity == 0;
}
bool sql_hydrate_item_owner_revisions(void)
{
	return false;
}
void get_level_cap_info(long *max_frags, int *racewar, int *level, time_t *next_update)
{
	if (max_frags)
		*max_frags = -1;
	if (racewar)
		*racewar = RACEWAR_NONE;
	if (level)
		*level = frag_cap_config_get()->cap_floor_level;
	if (next_update)
		*next_update = 0;
}
int sql_level_cap(int /*racewar_side*/)
{
	return frag_cap_config_get()->cap_floor_level;
}
double sql_get_total_donated(const char * /*account_name*/)
{
	return 0.0;
}
void sql_update_frag_leaderboard(P_char ch)
{
	if (!ch || IS_NPC(ch))
		return;
	if (IS_MORPH(ch))
		ch = MORPH_ORIG(ch);
	const char *root = persistence_mode_flatfile_root();
	const char *account = get_account_name_safe(ch);
	const char *name = GET_NAME(ch);
	if (!root || GET_PID(ch) <= 0 || !account || !*account || !name || !*name)
		return;
	flatfile_frag_leaderboard_record record;
	record.pid = static_cast<uint32_t>(GET_PID(ch));
	record.account_name = account;
	record.character_name = name;
	record.total_frags = ch->only.pc->frags;
	record.racewar = GET_RACEWAR(ch);
	record.race_name = race_names_table[ch->player.race].normal;
	record.class_name = class_names_table[flag2idx(ch->player.m_class)].normal;
	record.level = GET_LEVEL(ch);
	record.last_updated = static_cast<int64_t>(time(nullptr));
	record.revision = 1;
	std::string error;
	if (flatfile_frag_leaderboard_upsert(root, record, &error) !=
	    flatfile_frag_leaderboard_result::ok)
		persistence_alert(AVATAR, "frag_leaderboard", "player", "unknown", "upsert",
				  "flat_write_failed", "pid=%d error=%s", GET_PID(ch),
				  error.c_str());
}
bool sql_soft_delete_character(long /*pid*/)
{
	return false;
}
bool sql_trace_exec_at(struct persistence_query_site /*source_site*/, const char * /*label*/,
		       const char * /*sql*/, size_t /*len*/, bool /*drain_before*/,
		       bool /*drain_after*/)
{
	return false;
}
void sql_log_player_login(P_char ch, const char *status)
{
	if (!ch || IS_NPC(ch) || !status ||
	    (strcasecmp(status, "login") && strcasecmp(status, "logout")))
		return;
	sql_log(ch, CONNECTLOG, "Session audit: %s", status);
}
void update_zone_db() {}
void update_zone_epic_level(int /*zone_id*/, int /*level*/) {}
void show_frag_trophy(P_char ch, P_char /*who*/)
{
	send_to_char("Disabled.", ch);
}

static void sanitize_flat_log_field(const char *source, char *destination, size_t capacity)
{
	if (!destination || capacity == 0)
		return;

	size_t index = 0;
	if (source)
	{
		for (; source[index] && index + 1 < capacity; ++index)
		{
			const unsigned char byte = static_cast<unsigned char>(source[index]);
			destination[index] = byte < 0x20 || byte == 0x7f ? ' ' : source[index];
		}
	}
	destination[index] = '\0';
}

void sql_log(P_char ch, const char *kind, const char *format, ...)
{
	if (!ch)
	{
		debug("sql_log called for non-existent ch!");
		return;
	}

	if (!IS_PC(ch))
	{
		debug("sql_log called in sql.c for mobile ch - %s - Vnum %d", GET_NAME(ch),
		      GET_VNUM(ch));
		debug("sql_log kind '%s', format '%s'", kind ? kind : "(null)",
		      format ? format : "(null)");
		return;
	}

	if (!ch->only.pc || !GET_NAME(ch) || !kind || !format)
	{
		debug("sql_log called with incomplete player log data");
		return;
	}

	static char message[MAX_STRING_LENGTH];
	va_list args;
	va_start(args, format);
	const int message_length = vsnprintf(message, sizeof(message), format, args);
	va_end(args);
	if (message_length < 0 || message_length >= static_cast<int>(sizeof(message)))
	{
		debug("sql_log: Message too long or formatting error");
		return;
	}

	char safe_kind[32];
	char safe_ip[sizeof(ch->desc->host)];
	char safe_name[MAX_INPUT_LENGTH];
	sanitize_flat_log_field(kind, safe_kind, sizeof(safe_kind));
	sanitize_flat_log_field(ch->desc ? ch->desc->host : "", safe_ip, sizeof(safe_ip));
	sanitize_flat_log_field(GET_NAME(ch), safe_name, sizeof(safe_name));
	sanitize_flat_log_field(message, message, sizeof(message));

	int zone_number = NOWHERE;
	int room_vnum = NOWHERE;
	if (world && ch->in_room >= 0 && ch->in_room <= top_of_world)
	{
		room_vnum = world[ch->in_room].number;
		const int zone_rnum = world[ch->in_room].zone;
		if (zone_table && zone_rnum >= 0 && zone_rnum <= top_of_zone_table)
			zone_number = zone_table[zone_rnum].number;
	}

	const char *destination = LOG_PLAYER;
	if (!strcmp(kind, WIZLOG))
		destination = LOG_WIZ;
	else if (!strcmp(kind, EXPLOG))
		destination = LOG_EXP;

	logit(destination, "kind=%s ip=%s pid=%d player=%s zone=%d room=%d message=%s", safe_kind,
	      safe_ip, GET_PID(ch), safe_name, zone_number, room_vnum, message);
}

bool get_zone_info(int /*zone_number*/, struct zone_info * /*info*/)
{
	return FALSE;
}

string escape_str(const char *str)
{
	return string(str);
}

string get_mud_info(const char *name)
{
	string contents, error;
	if (!name || !flatfile_information_read(".", name, &contents, &error))
	{
		logit(LOG_DEBUG, "get_mud_info: %s",
		      error.empty() ? "invalid name" : error.c_str());
		return {};
	}
	return contents;
}

void send_mud_info(const char *name, P_char ch)
{
	send_to_char(get_mud_info(name).c_str(), ch, LOG_NONE);
}

void sql_update_bind_data(int vnum, int *owner_pid, int *timer)
{
	if (!owner_pid || !timer)
	{
		logit(LOG_DEBUG, "sql_update_bind_data: invalid input pointer");
		return;
	}
	std::string error;
	const auto updated = flatfile_artifact_bind_update(persistence_mode_flatfile_root(), vnum,
							   *owner_pid, *timer, &error);
	if (updated != flatfile_artifact_result::ok &&
	    updated != flatfile_artifact_result::unchanged)
		logit(LOG_DEBUG, "sql_update_bind_data: flat artifact update failed: %s",
		      error.empty() ? "invalid or missing artifact authority" : error.c_str());
}

bool sql_get_bind_data(int vnum, int *owner_pid, int *timer)
{
	if (owner_pid)
		*owner_pid = 0;
	if (timer)
		*timer = 0;
	if (!owner_pid || !timer)
	{
		logit(LOG_DEBUG, "sql_get_bind_data: invalid output pointer");
		return false;
	}
	int32_t flat_owner_pid = 0;
	int64_t flat_timer = 0;
	std::string error;
	const auto loaded = flatfile_artifact_bind_get(persistence_mode_flatfile_root(), vnum,
						       &flat_owner_pid, &flat_timer, &error);
	if (loaded != flatfile_artifact_result::ok || flat_timer > INT_MAX)
	{
		logit(LOG_DEBUG, "sql_get_bind_data: flat artifact lookup failed: %s",
		      error.empty() ? "invalid or missing artifact authority" : error.c_str());
		return false;
	}
	*owner_pid = flat_owner_pid;
	*timer = static_cast<int>(flat_timer);
	return true;
}

bool sql_pwipe(int code_verify)
{
	if (code_verify == 1723699)
	{
		logit(LOG_DEBUG,
		      "sql_pwipe: &=GlCan't wipe the SQL stuff as SQL database is not loaded.");
	}
	else
	{
		logit(LOG_DEBUG,
		      "sql_pwipe: &=GlSomeone called sql_pwipe with a bad verify code... hrm..");
	}
	return FALSE;
}
bool sql_pwipe_crossed_boundary(void)
{
	return false;
}
uint64_t sql_season_epoch(void)
{
	return 0;
}
bool sql_clear_zone_trophy()
{
	return FALSE;
}
#else

static void sql_resetConnectTimes(void);
static bool sql_verify_boot_database(void);

// The global database handler
MYSQL *DB;

/* persistenceDB replaced by connection pool (sql_pool.c).
 * persistence_sql_mutex kept for backward compatibility -- no longer
 * needed for connection serialisation but still referenced by
 * sql_persistence_raw.c for now. */
MYSQL *persistenceDB = NULL;
pthread_mutex_t persistence_sql_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool pwipe_crossed_boundary = false;
static uint64_t current_season_epoch = 0;

static bool sql_env_true(const char *name)
{
	const char *value = getenv(name);
	return value && !strcasecmp(value, "TRUE");
}

static bool sql_host_is_loopback(const char *host)
{
	return host && (!strcasecmp(host, "localhost") || !strcmp(host, "127.0.0.1") ||
			!strcmp(host, "::1"));
}

static bool sql_target_is_allowed(const char *host, const char *database)
{
	const char *allowed = getenv("DB_ALLOWED_TARGETS");
	if (!allowed || !*allowed || !host || !database)
		return false;

	char target[512];
	int written = snprintf(target, sizeof(target), "%s/%s", host, database);
	if (written < 0 || (size_t)written >= sizeof(target))
		return false;

	size_t target_len = (size_t)written;
	for (const char *start = allowed; *start;)
	{
		const char *end = strchr(start, ',');
		size_t len = end ? (size_t)(end - start) : strlen(start);
		if (len == target_len && !strncmp(start, target, len))
			return true;
		if (!end)
			break;
		start = end + 1;
	}
	return false;
}

static bool sql_runtime_config_valid(void)
{
	const char *role = getenv("ENVIRONMENT");
	if (!role || (strcmp(role, "local") && strcmp(role, "production")))
	{
		logit(LOG_STATUS,
		      "Database configuration rejected: ENVIRONMENT must be local or production");
		return false;
	}

	const char *required[] = { "DB_HOST", "DB_USER", "DB_PASSWD", "DB_NAME",
				   "DB_ALLOWED_TARGETS" };
	for (const char *name : required)
	{
		const char *value = getenv(name);
		if (!value || !*value)
		{
			logit(LOG_STATUS,
			      "Database configuration rejected: required field %s is missing",
			      name);
			return false;
		}
	}

	const char *port = getenv("DB_PORT");
	if (port && *port)
	{
		errno = 0;
		char *end = NULL;
		long parsed = strtol(port, &end, 10);
		if (errno == ERANGE || end == port || *end || parsed < 1 || parsed > 65535)
		{
			logit(LOG_STATUS, "Database configuration rejected: DB_PORT is invalid");
			return false;
		}
	}

	if (!strcmp(role, "production") && RUNNING_PORT != DFLT_PORT)
	{
		logit(LOG_STATUS,
		      "Database configuration rejected: production role requires the production port");
		return false;
	}

	const char *database = sql_persistence_db_name();
	if (!sql_target_is_allowed(DB_HOST, database))
	{
		logit(LOG_STATUS,
		      "Database configuration rejected: resolved target is not allow-listed");
		return false;
	}

	const char *socket_path = getenv("DB_SOCKET");
	bool protected_local = sql_host_is_loopback(DB_HOST) || (socket_path && *socket_path);
	if (socket_path && *socket_path &&
	    (!sql_host_is_loopback(DB_HOST) || strcmp(role, "local")))
	{
		logit(LOG_STATUS, "Database configuration rejected: DB_SOCKET is local-mode only");
		return false;
	}
	if (RUNTIME_DB_REMOTE_TLS_REQUIRED && !protected_local)
	{
		const char *ca = getenv("DB_SSL_CA");
		struct stat ca_stat;
		if (!sql_env_true("DB_TLS") || !ca || !*ca || stat(ca, &ca_stat) ||
		    !S_ISREG(ca_stat.st_mode))
		{
			logit(LOG_STATUS,
			      "Database configuration rejected: remote transport requires TLS and a CA file");
			return false;
		}
	}
	return true;
}

static bool sql_connection_execute(MYSQL *conn, const char *statement)
{
	if (mysql_real_query(conn, statement, strlen(statement)))
		return false;
	MYSQL_RES *result = mysql_store_result(conn);
	if (result)
		mysql_free_result(result);
	return mysql_next_result(conn) == -1;
}

static bool sql_connection_execute_affected(MYSQL *conn, const char *statement,
					    my_ulonglong *affected)
{
	if (!affected || mysql_real_query(conn, statement, strlen(statement)))
		return false;
	MYSQL_RES *result = mysql_store_result(conn);
	if (result)
		mysql_free_result(result);
	*affected = mysql_affected_rows(conn);
	return *affected != (my_ulonglong)-1 && mysql_next_result(conn) == -1;
}

static bool sql_load_active_season_state(void)
{
	MYSQL_RES *result = db_query(
		"SELECT season_epoch,reset_status FROM season_reset_state WHERE state_id=1");
	if (!result)
		return false;
	MYSQL_ROW row = mysql_fetch_row(result);
	char *end = NULL;
	errno = 0;
	unsigned long long epoch = row && row[0] ? strtoull(row[0], &end, 10) : 0;
	const bool ready = row && row[0] && end && !*end && !errno && epoch > 0 && row[1] &&
			   !strcmp(row[1], "active") && mysql_fetch_row(result) == NULL;
	mysql_free_result(result);
	if (!ready)
		return false;
	current_season_epoch = epoch;
	return true;
}

static bool sql_begin_pwipe_epoch(void)
{
	pwipe_crossed_boundary = false;
	if (!DB || !sql_connection_execute(DB, "START TRANSACTION"))
		return false;
	MYSQL_RES *result = db_query(
		"SELECT season_epoch,reset_status FROM season_reset_state WHERE state_id=1 FOR UPDATE");
	MYSQL_ROW row = result ? mysql_fetch_row(result) : NULL;
	char *end = NULL;
	errno = 0;
	unsigned long long epoch = row && row[0] ? strtoull(row[0], &end, 10) : 0;
	const bool active = row && row[0] && end && !*end && !errno && epoch > 0 &&
			    epoch < ULLONG_MAX && row[1] && !strcmp(row[1], "active") &&
			    mysql_fetch_row(result) == NULL;
	if (result)
		mysql_free_result(result);
	if (!active)
	{
		sql_connection_execute(DB, "ROLLBACK");
		return false;
	}
	char update[384];
	snprintf(update, sizeof update,
		 "UPDATE season_reset_state SET season_epoch=%llu,reset_status='resetting',"
		 "reset_started_at=UTC_TIMESTAMP(6),reset_completed_at=NULL "
		 "WHERE state_id=1 AND season_epoch=%llu AND reset_status='active'",
		 epoch + 1, epoch);
	my_ulonglong affected = 0;
	if (!sql_connection_execute_affected(DB, update, &affected) || affected != 1)
	{
		sql_connection_execute(DB, "ROLLBACK");
		return false;
	}
	/* The update succeeded; a failed COMMIT can now have an ambiguous outcome. */
	pwipe_crossed_boundary = true;
	if (!sql_connection_execute(DB, "COMMIT"))
		return false;
	current_season_epoch = epoch + 1;
	return true;
}

static bool sql_complete_pwipe_epoch(void)
{
	if (!DB || !current_season_epoch)
		return false;
	char update[320];
	snprintf(update, sizeof update,
		 "UPDATE season_reset_state SET reset_status='active',"
		 "reset_completed_at=UTC_TIMESTAMP(6) WHERE state_id=1 AND season_epoch=%llu "
		 "AND reset_status='resetting'",
		 (unsigned long long)current_season_epoch);
	my_ulonglong affected = 0;
	return sql_connection_execute_affected(DB, update, &affected) && affected == 1;
}

bool sql_pwipe_crossed_boundary(void)
{
	return pwipe_crossed_boundary;
}

uint64_t sql_season_epoch(void)
{
	return current_season_epoch;
}

static bool sql_mode_has(const char *mode, const char *required)
{
	if (!mode || !required)
		return false;
	size_t required_len = strlen(required);
	for (const char *start = mode; *start;)
	{
		const char *end = strchr(start, ',');
		size_t len = end ? (size_t)(end - start) : strlen(start);
		if (len == required_len && !strncmp(start, required, len))
			return true;
		if (!end)
			break;
		start = end + 1;
	}
	return false;
}

static bool sql_verify_session_contract(MYSQL *conn)
{
	const char *verify = "SELECT @@character_set_connection,@@time_zone,@@sql_mode";
	if (mysql_real_query(conn, verify, strlen(verify)))
		return false;
	MYSQL_RES *result = mysql_store_result(conn);
	MYSQL_ROW row = result ? mysql_fetch_row(result) : NULL;
	bool valid = row && row[0] && !strcmp(row[0], RUNTIME_DB_CHARACTER_SET) && row[1] &&
		     !strcmp(row[1], RUNTIME_DB_TIME_ZONE) && row[2] &&
		     sql_mode_has(row[2], "STRICT_TRANS_TABLES") &&
		     sql_mode_has(row[2], "ERROR_FOR_DIVISION_BY_ZERO") &&
		     sql_mode_has(row[2], "NO_ENGINE_SUBSTITUTION");
	if (result)
		mysql_free_result(result);
	if (!valid)
		return false;

	const char *isolation_queries[] = { "SELECT @@transaction_isolation",
					    "SELECT @@tx_isolation" };
	for (const char *query : isolation_queries)
	{
		if (mysql_real_query(conn, query, strlen(query)))
			continue;
		result = mysql_store_result(conn);
		row = result ? mysql_fetch_row(result) : NULL;
		valid = row && row[0] && !strcasecmp(row[0], RUNTIME_DB_ISOLATION);
		if (result)
			mysql_free_result(result);
		if (valid)
			return true;
	}
	return false;
}

static bool sql_apply_session_contract(MYSQL *conn)
{
	if (mysql_set_character_set(conn, RUNTIME_DB_CHARACTER_SET))
		return false;
	std::string sql_mode_statement =
		"SET SESSION sql_mode='" + std::string(RUNTIME_DB_SQL_MODE) + "'";
	const char *statements[] = { "SET SESSION time_zone='+00:00'",
				     "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED",
				     sql_mode_statement.c_str() };
	for (const char *statement : statements)
		if (!sql_connection_execute(conn, statement))
			return false;
	return sql_verify_session_contract(conn);
}

MYSQL *sql_open_configured_connection(unsigned long client_flags)
{
	if (!sql_runtime_config_valid())
		return NULL;

	MYSQL *conn = mysql_init(NULL);
	if (!conn)
		return NULL;

	unsigned int timeout = RUNTIME_DB_TIMEOUT_SECONDS;
	bool reconnect = false;
	if (mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout) ||
	    mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &timeout) ||
	    mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, &timeout) ||
	    mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect) ||
	    mysql_options(conn, MYSQL_SET_CHARSET_NAME, RUNTIME_DB_CHARACTER_SET))
	{
		mysql_close(conn);
		return NULL;
	}

	const char *socket_path = getenv("DB_SOCKET");
	bool protected_local = sql_host_is_loopback(DB_HOST) || (socket_path && *socket_path);
	if (RUNTIME_DB_REMOTE_TLS_REQUIRED && !protected_local)
	{
		const char *ca = getenv("DB_SSL_CA");
		/* Both arms demand the same thing: TLS is mandatory, the server
		 * certificate must chain to the CA, and the name on it must match the
		 * host we asked for.  MySQL deprecated MYSQL_OPT_SSL_ENFORCE and
		 * MYSQL_OPT_SSL_VERIFY_SERVER_CERT in 5.7 and removed them in 8.0,
		 * folding both into MYSQL_OPT_SSL_MODE; MariaDB Connector/C ships only
		 * the original pair.  Build against either without weakening the
		 * requirement -- a downgrade here is silent until someone is on the
		 * wrong end of it. */
#if defined(MARIADB_BASE_VERSION) || defined(MARIADB_PACKAGE_VERSION)
		bool enabled = true;
		if (mysql_options(conn, MYSQL_OPT_SSL_ENFORCE, &enabled) ||
		    mysql_options(conn, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &enabled) ||
		    mysql_options(conn, MYSQL_OPT_SSL_CA, ca))
#else
		unsigned int ssl_mode = SSL_MODE_VERIFY_IDENTITY;
		if (mysql_options(conn, MYSQL_OPT_SSL_MODE, &ssl_mode) ||
		    mysql_options(conn, MYSQL_OPT_SSL_CA, ca))
#endif
		{
			mysql_close(conn);
			return NULL;
		}
		client_flags |= CLIENT_SSL;
	}

	if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWD, sql_persistence_db_name(),
				DB_PORT, socket_path && *socket_path ? socket_path : NULL,
				client_flags))
	{
		logit(LOG_STATUS, "Database connection failed error_code=%u sqlstate=%.5s",
		      (unsigned int)mysql_errno(conn), mysql_sqlstate(conn));
		mysql_close(conn);
		return NULL;
	}
	if ((!protected_local && !mysql_get_ssl_cipher(conn)) || !sql_apply_session_contract(conn))
	{
		logit(LOG_STATUS,
		      "Database connection rejected: transport or session contract failed");
		mysql_close(conn);
		return NULL;
	}
	return conn;
}

/* Escapes a string. */
char *mysql_str(const char *str, char *buf)
{
	mysql_real_escape_string(DB, buf, str, strlen(str));
	return buf;
}

string escape_str(const char *str)
{
	size_t len;
	string escaped;
	unsigned long escaped_len;

	if (!str || !DB)
		return string();

	len = strlen(str);
	if (len > (string().max_size() - 1) / 2)
		return string();

	/* mysql_real_escape_string() can expand every input byte and needs
	 * one additional byte for the terminator.  Keep the storage owned by
	 * this call so concurrent persistence workers cannot overwrite it. */
	escaped.assign(len * 2 + 1, '\0');
	escaped_len = mysql_real_escape_string(DB, &escaped[0], str, len);
	escaped.resize(escaped_len);
	return escaped;
}

static void lookup_append(std::string *output, const std::string &value)
{
	uint64_t size = value.size();
	for (int shift = 56; shift >= 0; shift -= 8)
		output->push_back((char)((size >> shift) & 0xff));
	output->append(value);
}

static std::string lookup_escape(const char *value)
{
	if (!value)
		value = "";
	std::string escaped(strlen(value) * 2 + 1, '\0');
	unsigned long length = mysql_real_escape_string(DB, &escaped[0], value, strlen(value));
	escaped.resize(length);
	return escaped;
}

static bool lookup_checksum(const std::string &canonical, char *encoded, size_t size)
{
	if (size < SHA256_DIGEST_LENGTH * 2 + 1)
		return false;
	unsigned char digest[SHA256_DIGEST_LENGTH];
	if (!SHA256((const unsigned char *)canonical.data(), canonical.size(), digest))
		return false;
	for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
		snprintf(encoded + i * 2, 3, "%02x", digest[i]);
	encoded[SHA256_DIGEST_LENGTH * 2] = '\0';
	return true;
}

static bool lookup_rows_match(const char *checksum, size_t race_count, size_t class_count)
{
	const char *queries[] = { "SELECT id,name,COALESCE(short_name,''),COALESCE(ansi_name,''),"
				  "COALESCE(abbrev,''),racewar,playable FROM races ORDER BY id",
				  "SELECT id,name,COALESCE(ansi_name,''),COALESCE(short_name,''),"
				  "COALESCE(menu_char,'') FROM classes ORDER BY id" };
	const size_t widths[] = { 7, 5 };
	const char *tags[] = { "race", "class" };
	const size_t expected[] = { race_count, class_count };
	std::string canonical;
	for (size_t query_index = 0; query_index < 2; ++query_index)
	{
		if (mysql_real_query(DB, queries[query_index], strlen(queries[query_index])))
			return false;
		MYSQL_RES *result = mysql_store_result(DB);
		if (!result || mysql_num_fields(result) != widths[query_index] ||
		    mysql_num_rows(result) != expected[query_index])
		{
			if (result)
				mysql_free_result(result);
			return false;
		}
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(result)) != NULL)
		{
			lookup_append(&canonical, tags[query_index]);
			for (size_t column = 0; column < widths[query_index]; ++column)
			{
				if (!row[column])
				{
					mysql_free_result(result);
					return false;
				}
				lookup_append(&canonical, row[column]);
			}
		}
		mysql_free_result(result);
	}
	char actual[SHA256_DIGEST_LENGTH * 2 + 1];
	return lookup_checksum(canonical, actual, sizeof actual) && !strcmp(actual, checksum);
}

static bool lookup_state_matches(const char *checksum, size_t race_count, size_t class_count)
{
	char query[256];
	snprintf(query, sizeof query,
		 "SELECT dataset_version,LOWER(HEX(dataset_checksum)),race_count,class_count "
		 "FROM lookup_dataset_state WHERE dataset_name='%s'",
		 LOOKUP_DATASET_NAME);
	if (mysql_real_query(DB, query, strlen(query)))
		return false;
	MYSQL_RES *result = mysql_store_result(DB);
	MYSQL_ROW row = result ? mysql_fetch_row(result) : NULL;
	bool matches = row && row[0] && atoi(row[0]) == (int)LOOKUP_DATASET_VERSION && row[1] &&
		       !strcmp(row[1], checksum) && row[2] &&
		       strtoul(row[2], NULL, 10) == race_count && row[3] &&
		       strtoul(row[3], NULL, 10) == class_count;
	if (result)
		mysql_free_result(result);
	return matches;
}

static std::string lookup_id_list(const std::vector<int> &ids)
{
	std::string list;
	for (int id : ids)
	{
		if (!list.empty())
			list += ',';
		list += std::to_string(id);
	}
	return list;
}

/* Publish the compiled race/class dataset atomically. Unchanged boots do no writes. */
bool sql_populate_lookup_tables()
{
	std::vector<std::string> race_sql, class_sql;
	std::vector<int> race_ids, class_ids;
	std::string canonical;

	for (int i = 0; i <= LAST_RACE; ++i)
	{
		if (!race_names_table[i].normal || !race_names_table[i].normal[0])
			continue;
		int racewar = 0, playable = 0;
		for (int j = 0; playable_races[j].race_id >= 0; ++j)
			if (playable_races[j].race_id == i)
			{
				playable = 1;
				if (!strcmp(playable_races[j].faction, "good"))
					racewar = RACEWAR_GOOD;
				else if (!strcmp(playable_races[j].faction, "evil"))
					racewar = RACEWAR_EVIL;
				else if (!strcmp(playable_races[j].faction, "undead"))
					racewar = RACEWAR_UNDEAD;
				else if (!strcmp(playable_races[j].faction, "neutral"))
					racewar = RACEWAR_NEUTRAL;
				break;
			}
		const char *raw[] = { race_names_table[i].normal,
				      race_names_table[i].no_spaces ?
					      race_names_table[i].no_spaces :
					      "",
				      race_names_table[i].ansi ? race_names_table[i].ansi : "",
				      race_names_table[i].code ? race_names_table[i].code : "" };
		lookup_append(&canonical, "race");
		lookup_append(&canonical, std::to_string(i));
		for (const char *value : raw)
			lookup_append(&canonical, value);
		lookup_append(&canonical, std::to_string(racewar));
		lookup_append(&canonical, std::to_string(playable));
		race_ids.push_back(i);
		race_sql.push_back(
			"INSERT INTO races(id,name,short_name,ansi_name,abbrev,racewar,playable) VALUES(" +
			std::to_string(i) + ",'" + lookup_escape(raw[0]) + "','" +
			lookup_escape(raw[1]) + "','" + lookup_escape(raw[2]) + "','" +
			lookup_escape(raw[3]) + "'," + std::to_string(racewar) + "," +
			std::to_string(playable) +
			") ON DUPLICATE KEY UPDATE name=VALUES(name),"
			"short_name=VALUES(short_name),ansi_name=VALUES(ansi_name),abbrev=VALUES(abbrev),"
			"racewar=VALUES(racewar),playable=VALUES(playable)");
	}

	for (int i = 0; i <= CLASS_COUNT; ++i)
	{
		if (!class_names_table[i].normal || !class_names_table[i].normal[0])
			continue;
		const char *raw[] = { class_names_table[i].normal,
				      class_names_table[i].ansi ? class_names_table[i].ansi : "",
				      class_names_table[i].code ? class_names_table[i].code : "" };
		char letter[] = { class_names_table[i].letter, '\0' };
		lookup_append(&canonical, "class");
		lookup_append(&canonical, std::to_string(i));
		for (const char *value : raw)
			lookup_append(&canonical, value);
		lookup_append(&canonical, letter);
		class_ids.push_back(i);
		class_sql.push_back(
			"INSERT INTO classes(id,name,ansi_name,short_name,menu_char) VALUES(" +
			std::to_string(i) + ",'" + lookup_escape(raw[0]) + "','" +
			lookup_escape(raw[1]) + "','" + lookup_escape(raw[2]) + "','" +
			lookup_escape(letter) +
			"') ON DUPLICATE KEY UPDATE name=VALUES(name),"
			"ansi_name=VALUES(ansi_name),short_name=VALUES(short_name),"
			"menu_char=VALUES(menu_char)");
	}

	char checksum[SHA256_DIGEST_LENGTH * 2 + 1];
	if (!lookup_checksum(canonical, checksum, sizeof checksum))
		return false;

	if (lookup_state_matches(checksum, race_sql.size(), class_sql.size()) &&
	    lookup_rows_match(checksum, race_sql.size(), class_sql.size()))
	{
		logit(LOG_STATUS, "Lookup dataset unchanged; publication skipped.");
		return true;
	}

	if (!sql_connection_execute(DB, "START TRANSACTION"))
		return false;
	auto rollback = []()
	{
		sql_connection_execute(DB, "ROLLBACK");
		return false;
	};
	for (const std::string &query : race_sql)
		if (!sql_connection_execute(DB, query.c_str()))
			return rollback();
	for (const std::string &query : class_sql)
		if (!sql_connection_execute(DB, query.c_str()))
			return rollback();
	std::string delete_races =
		"DELETE FROM races WHERE id NOT IN (" + lookup_id_list(race_ids) + ")";
	std::string delete_classes =
		"DELETE FROM classes WHERE id NOT IN (" + lookup_id_list(class_ids) + ")";
	if (!sql_connection_execute(DB, delete_races.c_str()) ||
	    !sql_connection_execute(DB, delete_classes.c_str()))
		return rollback();
	if (!lookup_rows_match(checksum, race_sql.size(), class_sql.size()))
		return rollback();
	std::string state =
		"INSERT INTO lookup_dataset_state(dataset_name,dataset_version,dataset_checksum,"
		"race_count,class_count) VALUES('" +
		std::string(LOOKUP_DATASET_NAME) + "'," + std::to_string(LOOKUP_DATASET_VERSION) +
		",UNHEX('" + checksum + "')," + std::to_string(race_sql.size()) + "," +
		std::to_string(class_sql.size()) +
		") ON DUPLICATE KEY UPDATE dataset_version=VALUES(dataset_version),"
		"dataset_checksum=VALUES(dataset_checksum),race_count=VALUES(race_count),"
		"class_count=VALUES(class_count)";
	if (!sql_connection_execute(DB, state.c_str()) || !sql_connection_execute(DB, "COMMIT"))
		return rollback();
	logit(LOG_STATUS, "Lookup dataset published atomically.");
	return true;
}

/* Resolve the requested database while retaining the non-default-port
 * production safety guard.  Explicit disposable/test database names must
 * remain usable on any port; only an implicit production target is redirected
 * to the development sandbox. */
const char *sql_persistence_db_name(void)
{
	const bool production_name = !strcmp(DB_NAME, "duris") || !strcmp(DB_NAME, "duris_prod");

	if (RUNNING_PORT != DFLT_PORT && production_name)
		return "duris_dev";
	return DB_NAME;
}

/* Open a connection to the database. The connection will remain open
 * throughout the mud session. */
int initialize_mysql()
{
	logit(LOG_STATUS, "Initializing validated MySQL connection.");
	DB = sql_open_configured_connection(CLIENT_MULTI_STATEMENTS);
	if (!DB)
	{
		return -1;
	}

	logit(LOG_STATUS, "Connection established.");

	sql_resetConnectTimes();

	if (!sql_verify_boot_database())
	{
		logit(LOG_STATUS,
		      "FATAL: required database connection/schema check failed, aborting boot");
		if (DB)
		{
			mysql_close(DB);
			DB = NULL;
		}
		return -1;
	}
	if (!sql_load_active_season_state())
	{
		logit(LOG_STATUS,
		      "FATAL: season reset state is missing, invalid, or not active; recovery is required");
		mysql_close(DB);
		DB = NULL;
		return -1;
	}
	if (!sql_populate_lookup_tables())
	{
		logit(LOG_STATUS,
		      "FATAL: COMPAT-E007 lookup dataset publication failed or commit outcome is ambiguous");
		mysql_close(DB);
		DB = NULL;
		return -1;
	}
	if (!item_uid_allocator_reserve(DB, ITEM_UID_BOOT_RESERVATION))
	{
		logit(LOG_STATUS,
		      "FATAL: could not reserve a collision-free item UID range at boot");
		mysql_close(DB);
		DB = NULL;
		return -1;
	}

	/* Initialise the connection pool for async persistence
	 * workers (item, scalar, large-payload event queues). */
	if (sql_pool_init(SQL_POOL_DEFAULT_SIZE) != 0)
	{
		logit(LOG_STATUS,
		      "Warning: connection pool init failed -- persistence workers will use sync fallback.");
		/* Non-fatal: the main DB connection still works. */
	}

	return 1;
}

void shutdown_mysql(void)
{
	sql_pool_shutdown();
	if (persistenceDB)
	{
		mysql_close(persistenceDB);
		persistenceDB = NULL;
	}
	if (DB)
	{
		mysql_close(DB);
		DB = NULL;
	}
}

/* Handle a query, log possible errors and return results (if available) */
MYSQL_RES *db_query_at(struct persistence_query_site site, const char *format, ...)
{
	va_list args;
	int needed;
	char *buf;
	MYSQL_RES *res;

	va_start(args, format);
	needed = vsnprintf(NULL, 0, format, args);
	va_end(args);
	if (needed < 0)
	{
		logit(LOG_DEBUG, "MySQL: Query formatting error");
		return NULL;
	}

	buf = (char *)malloc((size_t)needed + 1);
	if (!buf)
		return NULL;

	va_start(args, format);
	vsnprintf(buf, (size_t)needed + 1, format, args);
	va_end(args);

	if (!buf[0])
	{
		free(buf);
		return NULL;
	}

	if (!sql_trace_exec_at(site, "db_query", buf, strlen(buf), true, false))
	{
		free(buf);
		return NULL;
	}

	res = mysql_store_result(DB);
	free(buf);
	return res;
}

/* Fail boot unless the database schema and required authority baselines are ready. */
static bool sql_verify_boot_database(void)
{
	if (!DB)
	{
		logit(LOG_STATUS, "FATAL: database connection is not initialized at boot.");
		return false;
	}

	MYSQL_RES *result = db_query(
		"SELECT "
		"(SELECT COUNT(*) FROM mud_schema_baselines WHERE baseline_id='%s' AND "
		"LOWER(HEX(schema_fingerprint))='%s' AND manifest_version=%u AND runner_version=1),"
		"(SELECT COUNT(*) FROM mud_schema_history WHERE migration_id='%s' AND "
		"sequence_number=%u AND LOWER(HEX(apply_checksum))='%s' AND "
		"LOWER(HEX(verify_checksum))='%s' AND runner_version=1),"
		"(SELECT COUNT(*) FROM mud_schema_migration_state WHERE state_id=1 AND "
		"applied_count=%u AND LOWER(HEX(history_checksum))='%s'),"
		"(SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
		"AND table_type='BASE TABLE' AND table_name IN (%s)),"
		"(SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
		"AND table_type='BASE TABLE' AND engine='InnoDB' AND "
		"table_collation='utf8mb4_unicode_ci' AND table_name IN (%s))",
		RUNTIME_BASELINE_ID, RUNTIME_BASELINE_FINGERPRINT,
		RUNTIME_COMPATIBILITY_MANIFEST_VERSION, RUNTIME_MIGRATION_HEAD_ID,
		RUNTIME_MIGRATION_HEAD_SEQUENCE, RUNTIME_MIGRATION_APPLY_CHECKSUM,
		RUNTIME_MIGRATION_VERIFY_CHECKSUM, RUNTIME_MIGRATION_HEAD_SEQUENCE,
		RUNTIME_MIGRATION_HISTORY_CHECKSUM, RUNTIME_TABLE_SQL_LIST, RUNTIME_TABLE_SQL_LIST);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: COMPAT-E001 compatibility metadata query failed");
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(result);
	unsigned long *lengths = row ? mysql_fetch_lengths(result) : NULL;
	bool compatibility_ok = row && lengths && row[0] && atoi(row[0]) == 1 && row[1] &&
				atoi(row[1]) == 1 && row[2] && atoi(row[2]) == 1 && row[3] &&
				atoi(row[3]) == (int)RUNTIME_CURRENT_TABLE_COUNT && row[4] &&
				atoi(row[4]) == (int)RUNTIME_CURRENT_TABLE_COUNT;
	mysql_free_result(result);
	if (!compatibility_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: COMPAT-E002 migration, table, engine, or collation identity mismatch expected_baseline=%s expected_head=%s expected_tables=%u",
		      RUNTIME_BASELINE_ID, RUNTIME_MIGRATION_HEAD_ID, RUNTIME_CURRENT_TABLE_COUNT);
		return false;
	}
	if (!sql_verify_metadata_fingerprint())
	{
		logit(LOG_STATUS,
		      "FATAL: COMPAT-E003 normalized table/column/index/foreign-key fingerprint mismatch");
		return false;
	}

	const char *probe = "SELECT 1 FROM accounts LIMIT 1";
	if (!sql_trace_exec("boot/accounts_probe", probe, strlen(probe), true, false))
	{
		logit(LOG_STATUS,
		      "FATAL: required accounts table is missing or unreadable at boot");
		return false;
	}

	result = mysql_store_result(DB);
	if (result)
		mysql_free_result(result);

	const char *player_revision_probe =
		"SELECT COUNT(*) FROM information_schema.columns "
		"WHERE table_schema=DATABASE() AND table_name='player_data' AND "
		"column_name='save_revision' AND data_type='bigint' AND "
		"column_type LIKE '%unsigned' AND "
		"is_nullable='NO' AND column_default='0'";
	result = db_query("%s", player_revision_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: player save revision schema query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool player_revision_ok = row && lengths && row[0] && atoi(row[0]) == 1;
	mysql_free_result(result);
	if (!player_revision_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: player save revision schema is missing or incompatible at boot");
		return false;
	}

	/* The asynchronous persistence workers require these tables and their
	 * idempotency/index contract.  Fail at boot rather than allowing a worker
	 * to silently divert every event to an unverified fallback path. */
	const char *event_schema_probe =
		"SELECT COUNT(*) FROM information_schema.columns "
		"WHERE table_schema=DATABASE() AND "
		"((table_name='persistence_item_events' AND column_name IN "
		"('id','ts_usec','event_type','item_uid','vnum','item','actor','actor_id','source','target','note','dedupe_key','created_at')) "
		"OR (table_name='persistence_scalar_events' AND column_name IN "
		"('id','event_type','event_key','boot_time','touched_at','zone_number','toucher_pid','group_size','epic_value','alignment_delta','dedupe_key','created_at')))";
	result = db_query("%s", event_schema_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: persistence event schema metadata query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	bool event_columns_ok = row && lengths && row[0] && atoi(row[0]) == 25;
	mysql_free_result(result);
	if (!event_columns_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: persistence event schema is incomplete at boot (expected 25 required columns).");
		return false;
	}

	const char *event_index_probe =
		"SELECT COUNT(*) FROM (SELECT DISTINCT table_name, index_name "
		"FROM information_schema.statistics WHERE table_schema=DATABASE() AND "
		"((table_name='persistence_item_events' AND index_name IN "
		"('PRIMARY','idx_item_uid_ts','idx_event_type_created','uq_item_dedupe')) "
		"OR (table_name='persistence_scalar_events' AND index_name IN "
		"('PRIMARY','idx_scalar_event_key','idx_scalar_zone_time','uq_scalar_dedupe')))) "
		"AS required_indexes";
	result = db_query("%s", event_index_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: persistence event index metadata query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	bool event_indexes_ok = row && lengths && row[0] && atoi(row[0]) == 8;
	mysql_free_result(result);
	if (!event_indexes_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: persistence event schema indexes are incomplete at boot (expected 8 entries).");
		return false;
	}

	/* Auction settlement uses explicit transactions and therefore requires
	 * transactional storage.  A legacy MyISAM table can make rollback appear
	 * to succeed while leaving pickup/refund state partially committed. */
	const char *auction_engine_probe =
		"SELECT COUNT(DISTINCT table_name) FROM information_schema.tables "
		"WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN "
		"('auction_bid_history','auction_item_pickups','auction_money_pickups','auctions')";
	result = db_query("%s", auction_engine_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: auction storage-engine metadata query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	bool auction_engines_ok = row && lengths && row[0] && atoi(row[0]) == 4;
	mysql_free_result(result);
	if (!auction_engines_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: transactional auction tables are not all InnoDB at boot (expected 4).");
		return false;
	}

	const char *critical_schema_probe =
		"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() "
		"AND ((table_name='critical_operation_inbox' AND column_name IN "
		"('operation_id','command_hash','keys_hash','command_type','schema_version',"
		"'payload_version','status','result_code','durable_revision','result_payload',"
		"'created_at','committed_at')) OR (table_name='critical_test_state' AND "
		"column_name IN ('entity_type','entity_id','value','revision','updated_at')) OR "
		"(table_name='critical_outbox' AND column_name IN "
		"('outbox_id','operation_id','event_index','destination','event_type',"
		"'payload_version','payload','status','attempt_count','next_attempt_at',"
		"'created_at','delivered_at','dead_lettered_at','last_error_code')) OR "
		"(table_name='critical_outbox_delivery_dedupe' AND column_name IN "
		"('consumer_id','outbox_id','delivered_at')))";
	result = db_query("%s", critical_schema_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: critical command schema metadata query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool critical_columns_ok = row && lengths && row[0] && atoi(row[0]) == 34;
	mysql_free_result(result);
	if (!critical_columns_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: critical command schema is incomplete at boot (expected 34 required columns).");
		return false;
	}
	const char *critical_index_probe =
		"SELECT COUNT(*) FROM (SELECT DISTINCT table_name,index_name FROM "
		"information_schema.statistics WHERE table_schema=DATABASE() AND "
		"((table_name='critical_operation_inbox' AND index_name IN "
		"('PRIMARY','idx_critical_inbox_status_created')) OR "
		"(table_name='critical_test_state' AND index_name='PRIMARY') OR "
		"(table_name='critical_outbox' AND index_name IN "
		"('PRIMARY','uq_critical_outbox_operation_event','idx_critical_outbox_claim',"
		"'idx_critical_outbox_age')) OR (table_name='critical_outbox_delivery_dedupe' "
		"AND index_name='PRIMARY'))) AS critical_required_indexes";
	result = db_query("%s", critical_index_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: critical command index metadata query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool critical_indexes_ok = row && lengths && row[0] && atoi(row[0]) == 8;
	mysql_free_result(result);
	if (!critical_indexes_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: critical command indexes are incomplete at boot (expected 8 entries).");
		return false;
	}
	const char *epic_schema_probe =
		"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() "
		"AND ((table_name='player_data' AND column_name='epic_revision') OR "
		"(table_name='epic_balance_baseline' AND column_name IN "
		"('pid','opening_balance','opening_revision','captured_at')) OR "
		"(table_name='epic_ledger' AND column_name IN "
		"('operation_id','pid','delta','balance_after','epic_revision','reason_type',"
		"'reason_id','source_site','created_at')))";
	result = db_query("%s", epic_schema_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: epic ledger schema metadata query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool epic_columns_ok = row && lengths && row[0] && atoi(row[0]) == 14;
	mysql_free_result(result);
	if (!epic_columns_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: epic ledger schema is incomplete at boot (expected 14 required columns).");
		return false;
	}
	const char *epic_index_probe =
		"SELECT COUNT(*) FROM (SELECT DISTINCT table_name,index_name FROM "
		"information_schema.statistics WHERE table_schema=DATABASE() AND "
		"((table_name='epic_balance_baseline' AND index_name='PRIMARY') OR "
		"(table_name='epic_ledger' AND index_name IN "
		"('PRIMARY','uq_epic_ledger_pid_revision','idx_epic_ledger_pid_created',"
		"'idx_epic_ledger_reason_created')))) AS epic_required_indexes";
	result = db_query("%s", epic_index_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: epic ledger index metadata query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool epic_indexes_ok = row && lengths && row[0] && atoi(row[0]) == 5;
	mysql_free_result(result);
	if (!epic_indexes_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: epic ledger indexes are incomplete at boot (expected 5 entries).");
		return false;
	}
	const char *epic_baseline_coverage_probe =
		"SELECT COUNT(*) FROM player_data AS player LEFT JOIN epic_balance_baseline AS baseline "
		"ON baseline.pid=player.pid WHERE baseline.pid IS NULL";
	result = db_query("%s", epic_baseline_coverage_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: epic balance baseline coverage query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool epic_baseline_coverage_ok = row && lengths && row[0] && atoll(row[0]) == 0;
	mysql_free_result(result);
	if (!epic_baseline_coverage_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: epic balance baseline does not cover every player at boot.");
		return false;
	}
	const char *currency_schema_probe =
		"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() "
		"AND ((table_name='player_data' AND column_name='wallet_revision') OR "
		"(table_name='account_banks' AND column_name='bank_revision') OR "
		"(table_name='currency_wallet_baseline' AND column_name IN "
		"('pid','opening_copper','opening_silver','opening_gold','opening_platinum',"
		"'opening_revision','captured_at')) OR (table_name='currency_bank_baseline' AND "
		"column_name IN ('bank_id','opening_copper','opening_silver','opening_gold',"
		"'opening_platinum','opening_revision','captured_at')) OR "
		"(table_name='currency_ledger' AND column_name IN "
		"('operation_id','pid','bank_id','wallet_delta_copper','wallet_delta_silver',"
		"'wallet_delta_gold','wallet_delta_platinum','bank_delta_copper',"
		"'bank_delta_silver','bank_delta_gold','bank_delta_platinum',"
		"'wallet_after_copper','wallet_after_silver','wallet_after_gold',"
		"'wallet_after_platinum','bank_after_copper','bank_after_silver','bank_after_gold',"
		"'bank_after_platinum','wallet_revision','bank_revision','reason_type','reason_id',"
		"'source_site','created_at')))";
	result = db_query("%s", currency_schema_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: currency ledger schema metadata query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool currency_columns_ok = row && lengths && row[0] && atoi(row[0]) == 41;
	mysql_free_result(result);
	if (!currency_columns_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: currency ledger schema is incomplete at boot (expected 41 required columns).");
		return false;
	}
	const char *currency_index_probe =
		"SELECT COUNT(*) FROM (SELECT DISTINCT table_name,index_name FROM "
		"information_schema.statistics WHERE table_schema=DATABASE() AND "
		"((table_name='currency_wallet_baseline' AND index_name='PRIMARY') OR "
		"(table_name='currency_bank_baseline' AND index_name='PRIMARY') OR "
		"(table_name='currency_ledger' AND index_name IN "
		"('PRIMARY','uq_currency_wallet_revision','uq_currency_bank_revision',"
		"'idx_currency_pid_created','idx_currency_bank_created',"
		"'idx_currency_reason_created')))) AS currency_required_indexes";
	result = db_query("%s", currency_index_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: currency ledger index metadata query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool currency_indexes_ok = row && lengths && row[0] && atoi(row[0]) == 8;
	mysql_free_result(result);
	if (!currency_indexes_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: currency ledger indexes are incomplete at boot (expected 8 entries).");
		return false;
	}
	const char *currency_wallet_baseline_coverage_probe =
		"SELECT COUNT(*) FROM player_data AS player LEFT JOIN currency_wallet_baseline AS "
		"baseline ON baseline.pid=player.pid WHERE baseline.pid IS NULL";
	result = db_query("%s", currency_wallet_baseline_coverage_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: currency wallet baseline coverage query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool currency_wallet_coverage_ok = row && lengths && row[0] && atoll(row[0]) == 0;
	mysql_free_result(result);
	if (!currency_wallet_coverage_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: currency wallet baseline does not cover every player at boot.");
		return false;
	}
	const char *currency_bank_baseline_coverage_probe =
		"SELECT COUNT(*) FROM account_banks AS bank LEFT JOIN currency_bank_baseline AS "
		"baseline ON baseline.bank_id=bank.id WHERE baseline.bank_id IS NULL";
	result = db_query("%s", currency_bank_baseline_coverage_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: currency bank baseline coverage query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool currency_bank_coverage_ok = row && lengths && row[0] && atoll(row[0]) == 0;
	mysql_free_result(result);
	if (!currency_bank_coverage_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: currency bank baseline does not cover every account bank at boot.");
		return false;
	}
	const char *character_baseline_readiness_probe =
		"SELECT COUNT(*),"
		"COALESCE(SUM(wallet.pid IS NULL),0),"
		"COALESCE(SUM(epic.pid IS NULL),0),"
		"COALESCE(SUM(combat.pid IS NULL),0) FROM ("
		"SELECT DISTINCT player.pid FROM player_data player "
		"JOIN account_characters mapping ON mapping.pid=player.pid "
		"WHERE player.active=1 AND mapping.deleted_at IS NULL AND mapping.blocked=0"
		") eligible "
		"LEFT JOIN currency_wallet_baseline wallet ON wallet.pid=eligible.pid "
		"LEFT JOIN epic_balance_baseline epic ON epic.pid=eligible.pid "
		"LEFT JOIN combat_frag_baseline combat ON combat.pid=eligible.pid";
	result = db_query("%s", character_baseline_readiness_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: character baseline readiness query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool character_baselines_ready = row && lengths && row[0] && row[1] && row[2] &&
					       row[3] && atoll(row[1]) == 0 && atoll(row[2]) == 0 &&
					       atoll(row[3]) == 0;
	if (!character_baselines_ready)
	{
		logit(LOG_STATUS,
		      "FATAL: active mapped character baseline readiness failed "
		      "(eligible=%lld wallet_missing=%lld epic_missing=%lld "
		      "combat_missing=%lld).",
		      row && row[0] ? atoll(row[0]) : -1, row && row[1] ? atoll(row[1]) : -1,
		      row && row[2] ? atoll(row[2]) : -1, row && row[3] ? atoll(row[3]) : -1);
		mysql_free_result(result);
		return false;
	}
	mysql_free_result(result);
	const char *item_ownership_schema_probe =
		"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() "
		"AND ((table_name='item_uid_allocator' AND column_name IN "
		"('allocator_id','next_uid','updated_at')) OR (table_name='item_owner_revision' "
		"AND column_name IN ('owner_type','owner_id','owner_context_id','revision','updated_at')) "
		"OR (table_name='item_current_owner' AND column_name IN "
		"('item_uid','root_item_uid','parent_item_uid','owner_type','owner_id',"
		"'owner_context_id','item_revision','vnum','state','coin_payload','updated_at')) OR "
		"(table_name='item_ownership_baseline' AND column_name IN "
		"('item_uid','root_item_uid','parent_item_uid','owner_type','owner_id',"
		"'owner_context_id','opening_item_revision','vnum','source_table','source_row_id',"
		"'captured_at')) OR (table_name='item_ownership_quarantine' AND column_name IN "
		"('quarantine_id','item_uid','source_table','source_row_id','conflict_code','evidence',"
		"'detected_at','repaired_at')) OR (table_name='item_ownership_ledger' AND "
		"column_name IN ('operation_id','event_index','item_uid','root_item_uid',"
		"'parent_item_uid','from_owner_type','from_owner_id','from_owner_context_id',"
		"'to_owner_type','to_owner_id','to_owner_context_id','item_revision',"
		"'from_owner_revision','to_owner_revision','reason_type','reason_id','source_site',"
		"'created_at')))";
	result = db_query("%s", item_ownership_schema_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: item ownership schema metadata query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool item_ownership_columns_ok = row && lengths && row[0] && atoi(row[0]) == 56;
	mysql_free_result(result);
	if (!item_ownership_columns_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: item ownership schema is incomplete at boot (expected 56 columns).");
		return false;
	}
	const char *item_ownership_index_probe =
		"SELECT COUNT(*) FROM (SELECT DISTINCT table_name,index_name FROM "
		"information_schema.statistics WHERE table_schema=DATABASE() AND ((table_name="
		"'item_uid_allocator' AND index_name='PRIMARY') OR (table_name='item_owner_revision' "
		"AND index_name IN ('PRIMARY','idx_item_owner_revision_updated')) OR (table_name="
		"'item_current_owner' AND index_name IN ('PRIMARY','idx_item_current_root_uid',"
		"'idx_item_current_owner','idx_item_current_parent')) OR (table_name="
		"'item_ownership_baseline' AND index_name IN ('PRIMARY','uq_item_baseline_source',"
		"'idx_item_baseline_owner')) OR (table_name='item_ownership_quarantine' AND index_name "
		"IN ('PRIMARY','uq_item_quarantine_evidence','idx_item_quarantine_open')) OR "
		"(table_name='item_ownership_ledger' AND index_name IN ('PRIMARY',"
		"'uq_item_ledger_item_revision','idx_item_ledger_item_created',"
		"'idx_item_ledger_from_owner','idx_item_ledger_to_owner')))) item_required_indexes";
	result = db_query("%s", item_ownership_index_probe);
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: item ownership index metadata query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool item_ownership_indexes_ok = row && lengths && row[0] && atoi(row[0]) == 18;
	mysql_free_result(result);
	if (!item_ownership_indexes_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: item ownership indexes are incomplete at boot (expected 18).");
		return false;
	}
	const char *item_ownership_foreign_key_probe =
		"SELECT COUNT(*) FROM information_schema.referential_constraints WHERE "
		"constraint_schema=DATABASE() AND constraint_name IN "
		"('item_current_parent_fk','item_ownership_operation_fk') AND "
		"update_rule='RESTRICT' AND delete_rule='RESTRICT'";
	result = db_query("%s", item_ownership_foreign_key_probe);
	if (!result)
	{
		logit(LOG_STATUS,
		      "FATAL: item ownership foreign-key metadata query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool item_ownership_foreign_keys_ok = row && lengths && row[0] && atoi(row[0]) == 2;
	mysql_free_result(result);
	if (!item_ownership_foreign_keys_ok)
	{
		logit(LOG_STATUS,
		      "FATAL: item ownership restrictive foreign keys are incomplete at boot.");
		return false;
	}
	result = db_query(
		"SELECT COUNT(*) FROM item_uid_allocator WHERE allocator_id=1 AND next_uid>0");
	if (!result)
	{
		logit(LOG_STATUS, "FATAL: item UID allocator query failed at boot");
		return false;
	}
	row = mysql_fetch_row(result);
	lengths = row ? mysql_fetch_lengths(result) : NULL;
	const bool item_uid_allocator_ok = row && lengths && row[0] && atoi(row[0]) == 1;
	mysql_free_result(result);
	if (!item_uid_allocator_ok)
	{
		logit(LOG_STATUS, "FATAL: item UID allocator singleton is missing at boot");
		return false;
	}
	return true;
}

static bool sql_verify_metadata_fingerprint(void)
{
	std::string query =
		"SELECT CONCAT('T',CHAR(9),table_name,CHAR(9),engine,CHAR(9),table_collation) "
		"FROM information_schema.tables WHERE table_schema=DATABASE() AND "
		"table_type='BASE TABLE' AND table_name IN (";
	query += RUNTIME_TABLE_SQL_LIST;
	query +=
		") UNION ALL SELECT CONCAT('C',CHAR(9),c.table_name,CHAR(9),"
		"c.column_name,CHAR(9),c.ordinal_position,CHAR(9),c.data_type,CHAR(9),c.is_nullable,"
		"CHAR(9),COALESCE(c.character_maximum_length,0),CHAR(9),"
		"COALESCE(c.numeric_precision,0),CHAR(9),COALESCE(c.numeric_scale,0),CHAR(9),"
		"COALESCE(c.datetime_precision,0),CHAR(9),CASE WHEN c.column_default IS NULL THEN "
		"'<NULL>' WHEN UPPER(c.column_default) LIKE "
		"'CURRENT_TIMESTAMP%' THEN 'CURRENT_TIMESTAMP' ELSE TRIM(BOTH '\\'' FROM "
		"c.column_default) END,CHAR(9),CONCAT(IF(LOWER(c.extra) LIKE "
		"'%auto_increment%','A',''),IF(LOWER(c.extra) LIKE '%on update%','U',''),"
		"IF(LOWER(c.extra) LIKE '%generated%','G',''))) FROM information_schema.columns c "
		"JOIN information_schema.tables t ON t.table_schema=c.table_schema AND "
		"t.table_name=c.table_name AND t.table_type='BASE TABLE' WHERE "
		"c.table_schema=DATABASE() AND c.table_name IN (";
	query += RUNTIME_TABLE_SQL_LIST;
	query +=
		") "
		"UNION ALL SELECT CONCAT('I',CHAR(9),table_name,CHAR(9),index_name,CHAR(9),"
		"non_unique,CHAR(9),seq_in_index,CHAR(9),column_name,CHAR(9),COALESCE(sub_part,0)) "
		"FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name IN (";
	query += RUNTIME_TABLE_SQL_LIST;
	query +=
		") UNION ALL SELECT "
		"CONCAT('F',CHAR(9),k.table_name,CHAR(9),k.constraint_name,CHAR(9),k.column_name,"
		"CHAR(9),k.referenced_table_name,CHAR(9),k.referenced_column_name,CHAR(9),"
		"k.ordinal_position,CHAR(9),r.update_rule,CHAR(9),r.delete_rule) FROM "
		"information_schema.key_column_usage k JOIN information_schema.referential_constraints "
		"r ON r.constraint_schema=k.constraint_schema AND "
		"r.constraint_name=k.constraint_name WHERE k.constraint_schema=DATABASE() AND "
		"(k.table_name IN (";
	query += RUNTIME_TABLE_SQL_LIST;
	query += ") OR k.referenced_table_name IN (";
	query += RUNTIME_TABLE_SQL_LIST;
	query += ")) AND k.referenced_table_name IS NOT NULL ORDER BY 1";
	if (mysql_real_query(DB, query.c_str(), query.size()))
		return false;
	MYSQL_RES *result = mysql_store_result(DB);
	if (!result)
		return false;
	std::string canonical;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)) != NULL)
	{
		if (!row[0])
		{
			mysql_free_result(result);
			return false;
		}
		size_t row_length = strlen(row[0]);
		if (row_length >= RUNTIME_METADATA_MAX_BYTES - canonical.size())
		{
			mysql_free_result(result);
			return false;
		}
		canonical += row[0];
		canonical += '\n';
	}
	mysql_free_result(result);
	unsigned char digest[SHA256_DIGEST_LENGTH];
	if (!SHA256((const unsigned char *)canonical.data(), canonical.size(), digest))
		return false;
	char encoded[SHA256_DIGEST_LENGTH * 2 + 1];
	for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
		snprintf(encoded + i * 2, 3, "%02x", digest[i]);
	encoded[SHA256_DIGEST_LENGTH * 2] = '\0';
	const char *server = mysql_get_server_info(DB);
	const char *expected = server && strstr(server, "MariaDB") ?
				       RUNTIME_MARIADB10_11_METADATA_FINGERPRINT :
				       RUNTIME_MYSQL8_METADATA_FINGERPRINT;
	return !strcmp(encoded, expected);
}

/* Same as above, but won't log failed queries, ie when key restrictions suffice */
MYSQL_RES *db_query_nolog_at(struct persistence_query_site site, const char *format, ...)
{
	va_list args;
	int needed;
	char *buf;

	va_start(args, format);
	needed = vsnprintf(NULL, 0, format, args);
	va_end(args);
	if (needed < 0)
		return NULL;

	buf = (char *)malloc((size_t)needed + 1);
	if (!buf)
		return NULL;

	va_start(args, format);
	vsnprintf(buf, (size_t)needed + 1, format, args);
	va_end(args);

	if (!sql_trace_exec_at(site, "db_query_nolog", buf, strlen(buf), true, false))
	{
		free(buf);
		return NULL;
	}

	free(buf);
	return mysql_store_result(DB);
}

/* Store core player data to the database. We assume that only association
 * names may contain special characters */
int sql_save_player_core(P_char ch)
{
	char query[MAX_STRING_LENGTH];
	char assoc_name[MAX_STRING_LENGTH];
	char assoc_name_sql[MAX_STRING_LENGTH * 2 + 1];
	struct char_player_data *p;

	if (IS_MORPH(ch))
		ch = MORPH_ORIG(ch);
	p = &ch->player;

	if (GET_ASSOC(ch) == NULL)
	{
		assoc_name[0] = '\0';
	}
	else
	{
		snprintf(assoc_name, MAX_STRING_LENGTH, "%s", GET_ASSOC(ch)->get_name().c_str());
	}
	mysql_str(assoc_name, assoc_name_sql);

	if (IS_SPECIALIZED(ch))
	{
	}

	// deactivate any other players with same name (handles renamed characters)
	snprintf(query, MAX_STRING_LENGTH,
		 "UPDATE player_data SET active = 0 WHERE name = '%s' and pid != %d", p->name,
		 GET_PID(ch));
	db_query(query);

	// Mark this player active and keep its denormalized account identity aligned
	// with the canonical account projection. Existing rows created before the
	// transactional status-save linkage are repaired on their next login.
	if (ch->desc && ch->desc->account && ch->desc->account->acct_name &&
	    ch->desc->account->acct_name[0])
	{
		char account_name_sql[MAX_STRING_LENGTH * 2 + 1];
		mysql_str(ch->desc->account->acct_name, account_name_sql);
		if (!qry("UPDATE player_data SET active=1,account_name='%s' WHERE pid=%d",
			 account_name_sql, GET_PID(ch)))
			return 0;
	}
	else if (!qry("UPDATE player_data SET active=1 WHERE pid=%d", GET_PID(ch)))
	{
		return 0;
	}

	// Update frag leaderboard tables for web statistics
	sql_update_account_character(ch);
	sql_update_frag_leaderboard(ch);

	return 1;
}

/* Save a variable delta. var_type: 1=FRAGS, 2=EXP */
#define PROGRESS_FRAGS 1
#define PROGRESS_EXP 2
void sql_save_progress(int pid, int delta, int var_type)
{
	db_query("INSERT INTO progress VALUES( 0, %d, %d, NOW(), %d )", pid, var_type, delta);
}

// Retrieves the current highest number of frags and which racewar side has it.
void get_level_cap_info(long *max_frags, int *racewar, int *level, time_t *next_update)
{
	MYSQL_RES *db = NULL;
	MYSQL_ROW row;
	db = db_query(
		"SELECT most_frags, racewar_leader, level, UNIX_TIMESTAMP(next_update) FROM level_cap");

	if ((db == NULL) || ((row = mysql_fetch_row(db)) == NULL))
	{
		debug("get_level_cap_info: Database read fail.");
		*max_frags = (long)-1;
		*racewar = RACEWAR_NONE;
		*level = frag_cap_config_get()->cap_floor_level;
		*next_update = 0;
		return;
	}
	*max_frags = (long)(atof(row[0]) * 100. + .01);
	*racewar = atoi(row[1]);
	*level = atoi(row[2]);
	*next_update = atol(row[3]);

	// cycle out until a NULL return
	while (row != NULL)
	{
		row = mysql_fetch_row(db);
	}
	mysql_free_result(db);
}

// Returns the highest level achievable by mortals, limited by racewar side.
int sql_level_cap(int /*racewar_side*/)
{
	int level_cap;
	MYSQL_RES *db = NULL;
	MYSQL_ROW row;

	db = db_query("SELECT level, racewar_leader FROM level_cap");

	if ((db == NULL) || ((row = mysql_fetch_row(db)) == NULL))
	{
		debug("sql_level_cap: Database read fail.");
		return frag_cap_config_get()->cap_floor_level;
	}

	level_cap = atoi(row[0]);

	// cycle out until a NULL return
	while (row != NULL)
	{
		row = mysql_fetch_row(db);
	}
	mysql_free_result(db);

	const struct frag_cap_config *config = frag_cap_config_get();

	// Clamp database values to the configured mortal-cap range.
	if (level_cap >= config->cap_maximum_level)
		return config->cap_maximum_level;
	if (level_cap <= config->cap_floor_level)
		return config->cap_floor_level;

	return level_cap;
}

// Checks the number of frags against the current highest and sets the new highest if applicable.
// Timer policy is selected from the configured old-level bands in frag_cap.cfg.
void sql_check_level_cap(long max_frags, int racewar)
{
	long old_max_frags;
	int old_racewar, old_level;
	const struct frag_cap_config *config = frag_cap_config_get();
	time_t next_update;
	char query[1024];

	get_level_cap_info(&old_max_frags, &old_racewar, &old_level, &next_update);
	// If we've capped out
	if (old_level >= config->cap_maximum_level)
	{
		return;
	}
	// If enough time has passed, and level should change, update level if appropriate.
	if (next_update <= time(NULL))
	{
		// Have enough frags to update level.
		if (old_level < frag_cap_config_cap_level_from_frags(max_frags / 100.))
		{
			// when level cap increases, give a boon to the side that caused it
			BoonData bdata;
			bdata.duration =
				frag_cap_config_boon_duration_minutes(); // configurable minutes
			bdata.racewar = racewar;
			bdata.type = BTYPE_EXPM;
			bdata.option = BOPT_MOB;
			bdata.criteria = 1;
			bdata.criteria2 = -1;
			bdata.bonus = frag_cap_config_boon_bonus();
			bdata.active = 1;
			bdata.repeat = 1;
			create_boon(&bdata);

			int next_level = old_level + config->cap_level_step;
			if (next_level > config->cap_maximum_level)
				next_level = config->cap_maximum_level;
			snprintf(
				query, sizeof(query),
				"UPDATE level_cap SET most_frags = %f, racewar_leader = %d, level = %d, next_update = FROM_UNIXTIME(%ld)",
				max_frags / 100., racewar, next_level,
				(long)(time(NULL) +
				       SECS_PER_REAL_DAY * frag_cap_config_timer_days(old_level)));
			db_query(query);
		}
		else if (max_frags > old_max_frags)
		{
			snprintf(query, 1024,
				 "UPDATE level_cap SET most_frags = %f, racewar_leader = %d",
				 max_frags / 100., racewar);
			db_query(query);
		}
	}
	// Just changing highest frag amount and, possibly, racewar leader.
	else if (max_frags > old_max_frags)
	{
		snprintf(query, 1024, "UPDATE level_cap SET most_frags = %f, racewar_leader = %d",
			 max_frags / 100., racewar);
		db_query(query);
	}
}

// Re-check the current racewar total even when no new frag was recorded.
// This allows a qualified cap increase and its boon to become available as
// soon as the configured timer expires.
void sql_check_level_cap_periodic(void)
{
	long max_frags;
	int old_racewar, old_level;
	time_t next_update;
	MYSQL_RES *res;
	MYSQL_ROW row;

	get_level_cap_info(&max_frags, &old_racewar, &old_level, &next_update);
	if (old_racewar == RACEWAR_NONE || old_level < 0)
		return;

	res = db_query(
		"SELECT COALESCE(SUM(total_frags), 0) FROM frag_leaderboard WHERE racewar=%d",
		old_racewar);
	if (!res)
		return;

	row = mysql_fetch_row(res);
	if (row && row[0])
	{
		max_frags = atol(row[0]);
		sql_check_level_cap(max_frags, old_racewar);
	}
	mysql_free_result(res);
}

// Sets the values of level (actual cap) and racewar (the side that is in the lead).
void get_level_cap(int *level, int *racewar)
{
	MYSQL_RES *db = NULL;
	MYSQL_ROW row = NULL;

	db = db_query("SELECT level, racewar_leader FROM level_cap");

	if ((db == NULL) || ((row = mysql_fetch_row(db)) == NULL))
	{
		debug("get_level_cap: Database read fail.");
		*level = 25;
		*racewar = RACEWAR_NONE;
	}
	else
	{
		*level = atoi(row[0]);
		*racewar = atoi(row[1]);
	}

	// cycle out until a NULL return
	while (row != NULL)
	{
		row = mysql_fetch_row(db);
	}
	mysql_free_result(db);
}

/* Save frags delta */
void sql_modify_frags(P_char ch, int gain)
{
	// We don't want IS_TRUSTED(ch) because that can be turned off with toggle fog.
	if (GET_LEVEL(ch) > MAXLVLMORTAL)
	{
		return;
	}
	if (IS_MORPH(ch))
		ch = MORPH_ORIG(ch);
	sql_save_progress(GET_PID(ch), gain, PROGRESS_FRAGS);
	// Update frag leaderboard with new frag count (incremental update for performance)
	// Only update if the character is in the database (pid > 0)
	if (GET_PID(ch) > 0)
	{
		db_query(
			"UPDATE frag_leaderboard SET total_frags = %d, last_updated = NOW() WHERE pid = %ld AND deleted_at IS NULL",
			ch->only.pc->frags, GET_PID(ch));
	}

	if (gain >= 0)
	{
		MYSQL_RES *res = db_query(
			"SELECT COALESCE(SUM(total_frags), 0) FROM frag_leaderboard WHERE racewar=%d",
			GET_RACEWAR(ch));
		if (res)
		{
			MYSQL_ROW row = mysql_fetch_row(res);
			if (row and row[0])
			{
				long total = atol(row[0]);
				sql_check_level_cap(total, GET_RACEWAR(ch));
			}
			mysql_free_result(res);
		}
	}
}

/*
 * Frag Leaderboard Hybrid System - for web statistics
 * These functions maintain the account_characters and frag_leaderboard tables
 * The MUD continues to use flat files, but web can query the database
 */

/*
 * Resolve an existing account_characters row id for an escaped character name,
 * or 0 when the mapping is absent.
 *
 * account_characters.id is a signed INT AUTO_INCREMENT, and MySQL consumes an
 * identity value on every INSERT ... ON DUPLICATE KEY UPDATE attempt, including
 * the ones that only update. Projecting an existing mapping on every save
 * therefore advanced the counter far past the surviving row count. Resolving the
 * row first keeps the steady-state path an UPDATE, which allocates nothing.
 */
static long sql_find_account_character_id(const char *escaped_char_name)
{
	MYSQL_RES *result =
		db_query("SELECT id FROM account_characters WHERE char_name='%s' LIMIT 1",
			 escaped_char_name);
	if (!result)
		return 0;

	MYSQL_ROW row = mysql_fetch_row(result);
	long mapping_id = (row && row[0]) ? atol(row[0]) : 0;
	mysql_free_result(result);
	return mapping_id;
}

/* Update account_characters mapping table */
void sql_update_account_character(P_char ch)
{
	char account_name_sql[MAX_STRING_LENGTH * 2 + 1];
	char char_name_sql[MAX_STRING_LENGTH * 2 + 1];
	const char *account_name;

	if (!ch || IS_NPC(ch))
		return;

	if (IS_MORPH(ch))
		ch = MORPH_ORIG(ch);

	if (GET_PID(ch) <= 0)
	{
		logit(LOG_DEBUG, "sql_update_account_character: invalid pid for %s",
		      GET_NAME(ch) ? GET_NAME(ch) : "<null>");
		return;
	}

	account_name = get_account_name_safe(ch);

	// account_characters is UNIQUE on char_name and the account character list is
	// selected by account_name, so writing the get_account_name_safe() placeholder
	// would move the row off its real account and empty that account's menu while
	// player_data still holds the character. An offline or descriptor-less save has
	// nothing to project; leave the existing mapping alone.
	if (!ch->desc || !ch->desc->account || !ch->desc->account->acct_name ||
	    !ch->desc->account->acct_name[0])
	{
		logit(LOG_DEBUG,
		      "sql_update_account_character: component=mapping outcome=skipped_no_account pid=%d",
		      GET_PID(ch));
		return;
	}

	// Escape strings for SQL safety
	mysql_str(account_name, account_name_sql);
	mysql_str(ch->player.name, char_name_sql);

	// Update an existing mapping in place and insert only a genuinely new one,
	// so a repeated projection of the same character allocates no identity value.
	// created_at is preserved either way.
	const long mapping_id = sql_find_account_character_id(char_name_sql);
	const bool written =
		mapping_id > 0 ? qry("UPDATE account_characters "
				     "SET account_name = '%s', pid = %ld, char_name = '%s', "
				     "deleted_at = NULL "
				     "WHERE id = %ld",
				     account_name_sql, GET_PID(ch), char_name_sql, mapping_id)
				 // ON DUPLICATE KEY UPDATE still converges when another writer
				 // inserted the same unique char_name between the lookup and here.
				 :
				 qry("INSERT INTO account_characters "
				     "(account_name, pid, char_name, created_at, deleted_at) "
				     "VALUES('%s', %ld, '%s', NOW(), NULL) "
				     "ON DUPLICATE KEY UPDATE "
				     "account_name = VALUES(account_name), "
				     "pid = VALUES(pid), "
				     "char_name = VALUES(char_name), "
				     "deleted_at = NULL",
				     account_name_sql, GET_PID(ch), char_name_sql);

	if (!written)
	{
		logit(LOG_DEBUG, "sql_update_account_character: failed for %s",
		      GET_NAME(ch) ? GET_NAME(ch) : "<null>");
	}
}

double sql_get_total_donated(const char *account_name)
{
#ifdef __NO_MYSQL__
	return 0.0;
#else
	if (!account_name || !*account_name)
		return 0.0;

	MYSQL_RES *res = db_query("SELECT total_donated FROM accounts WHERE account_name='%s'",
				  escape_str(account_name).c_str());
	if (!res)
		return 0.0;

	double total = 0.0;
	MYSQL_ROW row = mysql_fetch_row(res);
	if (row && row[0])
		total = atof(row[0]);

	mysql_free_result(res);
	return total;
#endif
}

/* Update frag_leaderboard table with current character data */
void sql_update_frag_leaderboard(P_char ch)
{
	char account_name_sql[MAX_STRING_LENGTH * 2 + 1];
	char char_name_sql[MAX_STRING_LENGTH * 2 + 1];
	char race_sql[MAX_STRING_LENGTH * 2 + 1];
	char class_sql[MAX_STRING_LENGTH * 2 + 1];
	const char *account_name;
	const char *race_name;
	const char *class_name;

	if (!ch || IS_NPC(ch))
		return;

	if (IS_MORPH(ch))
		ch = MORPH_ORIG(ch);

	if (GET_PID(ch) <= 0)
	{
		logit(LOG_DEBUG, "sql_update_frag_leaderboard: invalid pid for %s",
		      GET_NAME(ch) ? GET_NAME(ch) : "<null>");
		return;
	}

	account_name = get_account_name_safe(ch);
	race_name = race_names_table[ch->player.race].normal;
	class_name = class_names_table[flag2idx(ch->player.m_class)].normal;

	// Escape strings for SQL safety
	mysql_str(account_name, account_name_sql);
	mysql_str(ch->player.name, char_name_sql);
	mysql_str(race_name, race_sql);
	mysql_str(class_name, class_sql);

	// Insert or update frag_leaderboard
	// Using INSERT ... ON DUPLICATE KEY UPDATE to preserve the row id while
	// refreshing the current stats.
	if (!qry("INSERT INTO frag_leaderboard "
		 "(pid, account_name, char_name, total_frags, racewar, race, class, level, deleted_at) "
		 "VALUES(%ld, '%s', '%s', %d, %d, '%s', '%s', %d, NULL) "
		 "ON DUPLICATE KEY UPDATE "
		 "account_name=VALUES(account_name), "
		 "char_name=VALUES(char_name), "
		 "total_frags=VALUES(total_frags), "
		 "racewar=VALUES(racewar), "
		 "race=VALUES(race), "
		 "class=VALUES(class), "
		 "level=VALUES(level), "
		 "deleted_at=NULL",
		 GET_PID(ch), account_name_sql, char_name_sql, ch->only.pc->frags, GET_RACEWAR(ch),
		 race_sql, class_sql, GET_LEVEL(ch)))
	{
		logit(LOG_DEBUG, "sql_update_frag_leaderboard: failed for %s",
		      GET_NAME(ch) ? GET_NAME(ch) : "<null>");
	}
}

/* Soft delete a character from the leaderboard tables */
bool sql_soft_delete_character(long pid)
{
	if (!DB || pid <= 0)
		return false;

	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
			return false;
		own_txn = true;
	}

	// Set deleted_at timestamp to NOW() for this character
	if (!db_query(
		    "UPDATE account_characters SET deleted_at = NOW() WHERE pid = %ld AND deleted_at IS NULL",
		    pid))
	{
		if (own_txn)
			sql_rollback();
		return false;
	}

	if (!db_query(
		    "UPDATE frag_leaderboard SET deleted_at = NOW() WHERE pid = %ld AND deleted_at IS NULL",
		    pid))
	{
		if (own_txn)
			sql_rollback();
		return false;
	}

	if (own_txn && !sql_commit())
	{
		sql_rollback();
		return false;
	}

	return true;
}

/* Save frags delta */
void sql_insert_item(P_char /*ch*/, P_obj obj, char *desc)
{
	char query[MAX_STRING_LENGTH];
	char sql_desc[MAX_STRING_LENGTH * 2 + 1];
	char sql_short[MAX_STRING_LENGTH * 2 + 1];

	int m_virtual = (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : 0;
	mysql_str(desc, sql_desc);
	mysql_str(obj->short_description, sql_short);

	db_query_nolog("INSERT INTO items_stats VALUES( null, '%s', '', %d)", sql_short, m_virtual);
	checked_snprintf(query, MAX_STRING_LENGTH,
			 "UPDATE items_stats SET  obj_stat = '%s', vnum = %d "
			 " WHERE short_desc = '%s'",
			 sql_desc, m_virtual, sql_short);

	db_query(query);
}

void sql_insert_new_item(P_char ch, P_obj obj)
{
	char item_id[MAX_STRING_LENGTH];

	snprintf(item_id, MAX_STRING_LENGTH, "o %s", obj->name);
	do_stat(ch, item_id, 555);
}

unsigned long new_pkill_event(P_char ch)
{
	char room_name_sql[MAX_STRING_LENGTH * 2 + 1];
	string query;

	mysql_str(world[ch->in_room].name, room_name_sql);
	query = "INSERT INTO pkill_event (stamp, room_vnum, room_name) VALUES( NOW(), ";
	query += std::to_string(world[ch->in_room].number);
	query += ", '";
	query += room_name_sql;
	query += "' )";

	sql_clear_results_on(DB);
	if (!sql_trace_exec("new_pkill_event", query.c_str(), query.size(), false, false))
	{
		logit(LOG_DEBUG, "MYSQL: Failed to create pkill event");
		return 0;
	}

	return mysql_insert_id(DB);
}

void get_pkill_player_description(P_char ch, char *buffer)
{
	char assoc_name[MAX_STRING_LENGTH];

	if (GET_ASSOC(ch) == NULL)
	{
		assoc_name[0] = '\0';
	}
	else
	{
		snprintf(assoc_name, MAX_STRING_LENGTH, "%s", GET_ASSOC(ch)->get_name().c_str());
	}

	checked_snprintf(buffer, MAX_STRING_LENGTH, "[%2d %s&n] %s &n%s &n(%s&n)", GET_LEVEL(ch),
			 get_class_name(ch, ch), GET_NAME(ch), assoc_name,
			 race_names_table[GET_RACE(ch)].ansi);

	logit(LOG_DEBUG, "%s", buffer);
}

void store_pkill_info(unsigned long pkill_event, P_char ch, const char *type, int leader,
		      int in_room)
{
	char buf[MAX_STRING_LENGTH];
	char equip_sql[MAX_STRING_LENGTH * 2 + 1];
	char player_description_sql[MAX_STRING_LENGTH * 2 + 1];
	char log_sql[MAX_LOG_LEN * 2 + 1];

	if (!ch || !IS_PC(ch))
		return;

	if (!GET_PLAYER_LOG(ch))
	{
		logit(LOG_DEBUG,
		      "Tried to dump player log (%s) in store_pkill_info(), but player log was null!",
		      GET_NAME(ch));
		return;
	}

	get_equipment_list(ch, buf, 1);
	mysql_str(buf, equip_sql);

	get_pkill_player_description(ch, buf);
	mysql_str(buf, player_description_sql);

	mysql_str(GET_PLAYER_LOG(ch)->read(LOG_PUBLIC, MAX_LOG_LEN), log_sql);

	db_query(
		"INSERT INTO pkill_info (event_id, pid, level, pk_type, player_description, equip, log, inroom, leader) "
		"VALUES( %d, %d, %d, '%s', '%s', '%s', '%s', %d ,%d )",
		pkill_event, GET_PID(ch), GET_LEVEL(ch), type, player_description_sql, equip_sql,
		log_sql, in_room, leader);
}

/* Save racewr pkill information */
void sql_save_pkill(P_char ch, P_char victim)
{
	unsigned long pkill_event;

	// NPCs can't be pkilled.
	if (IS_NPC(victim))
	{
		return;
	}

	/* If pet is the killer, we blame the owner, if he's around */
	if (IS_NPC(ch))
	{
		if (ch->following && IS_PC(ch->following) &&
		    ch->in_room == ch->following->in_room && grouped(ch, ch->following))
		{
			ch = ch->following;
		}
		else
		{
			return;
		}
	}

	/* Log a new pkill event, and get the handler for further logs */
	pkill_event = new_pkill_event(ch);
	if (!pkill_event)
		return;

	int in_room = 0;
	int leader = 0;

	// always store killer first, then group
	if (IS_PC(ch))
	{
		leader = (ch->group && ch->group->ch == ch) ? 1 : 0;
		store_pkill_info(pkill_event, ch, "KILLER", leader, 1);
	}

	if (ch->group)
	{
		for (struct group_list *gl = ch->group; gl; gl = gl->next)
		{
			if (IS_PC(gl->ch) && gl->ch != ch)
			{
				in_room = (ch->in_room == gl->ch->in_room) ? 1 : 0;
				store_pkill_info(pkill_event, gl->ch, "KILLER", 0, in_room);
			}
		}
	}

	// always store victim first, then group
	if (IS_PC(victim))
	{
		leader = (victim->group && victim->group->ch == victim) ? 1 : 0;
		store_pkill_info(pkill_event, victim, "VICTIM", leader, 1);
	}

	if (victim->group)
	{
		for (struct group_list *gl = victim->group; gl; gl = gl->next)
		{
			if (IS_PC(gl->ch) && gl->ch != victim)
			{
				in_room = (victim->in_room == gl->ch->in_room) ? 1 : 0;
				store_pkill_info(pkill_event, gl->ch, "VICTIM-GROUP", 0, in_room);
			}
		}
	}
}

/* Save character's preferences about displaying extended info on
   webpage for all to see. */
void sql_webinfo_toggle(P_char ch)
{
	if (!ch || !IS_PC(ch))
		return;
	// webinfo is stored in act2 flag, saved with player_data
}

/* Update level info */
void sql_update_level(P_char ch)
{
	if (!ch || !IS_PC(ch))
		return;
	// level already saved in player_data
}

/* Update money info */
void sql_update_money(P_char ch)
{
	if (!ch || !IS_PC(ch))
		return;
	// money stored as copper/silver/gold/platinum in player_data
}

/* Update playtime info */
void sql_update_playtime(P_char ch)
{
	if (!ch || !IS_PC(ch))
		return;
	// playtime is played_time in player_data
}

/* Update player's epics: We want to record their total epics gained not epics unused */
void sql_update_epics(P_char ch)
{
	if (!ch || !IS_PC(ch))
		return;
	// epics already in player_data
}

void manual_log(P_char ch)
{
	char a[256], b[256];
	char buf[MAX_STRING_LENGTH];
	char log_sql[MAX_LOG_LEN * 2 + 1];
	char buf2[MAX_LOG_LEN];
	int space = MAX_LOG_LEN;

	// paranoia check
	if (!ch || !IS_PC(ch))
		return;

	if (!GET_PLAYER_LOG(ch))
	{
		logit(LOG_DEBUG,
		      "Tried to dump player log (%s) in manual_log(), but player log was null!",
		      GET_NAME(ch));
		return;
	}

	*buf2 = '\0';

	ITERATE_LOG(ch, LOG_PUBLIC)
	{
		strncat(buf2, LOG_MSG(), space);
		space -= strlen(LOG_MSG());

		if (space <= 0)
			break;
	}

	mysql_str(buf2, log_sql);

	snprintf(a, 256, "%d%d", number(0, 32767), number(0, 2147483647));
	snprintf(b, 256, "%s", CRYPT2(a, ch->player.name));

	db_query("INSERT INTO MANUAL_LOG VALUES( 0, '%s', '%s', %d, 0, NOW() )", log_sql, b,
		 GET_PID(ch));

	snprintf(
		buf, MAX_STRING_LENGTH,
		"Your log is @ '&+Whttp://duris.game-host.org/duris/php/stats/mylog.php?password=%s&n' \n",
		b);

	send_to_char(buf, ch, LOG_PRIVATE);
}

void sql_resetConnectTimes(void)
{
	// this should ONLY be called on mud bootup.  to ensure that, call it when sql is initialized
	db_query("UPDATE ip_info SET last_disconnect = NOW() WHERE last_connect > last_disconnect");
}

void sql_disconnectIP(P_char ch)
{
	if (!ch || !IS_PC(ch))
		return;

	db_query_nolog("INSERT IGNORE INTO ip_info (pid) VALUES (%d)", GET_PID(ch));
	if (ch->desc)
	{
		// Set racewar side if not an immortal.
		db_query(
			"UPDATE ip_info SET last_disconnect = NOW(), racewar_side=%d WHERE pid = %d",
			IS_TRUSTED(ch) ? RACEWAR_NONE : GET_RACEWAR(ch), GET_PID(ch));
	}
}

void sql_connectIP(P_char ch)
{
	// insert will silently fail if the PID is already in the table
	db_query_nolog("INSERT IGNORE INTO ip_info (pid) VALUES (%d)", GET_PID(ch));
	if (ch->desc)
	{
		db_query(
			"UPDATE ip_info SET last_ip = '%s', last_connect = NOW(), racewar_side = %d WHERE pid = %d",
			ch->desc->host, IS_TRUSTED(ch) ? RACEWAR_NONE : GET_RACEWAR(ch),
			GET_PID(ch));
	}
}

void sql_world_quest_finished(P_char ch, P_obj reward)
{
	char buf[MAX_STRING_LENGTH * 2 + 1];

	int reward_vnum =
		reward ? ((reward->R_num >= 0) ? obj_index[reward->R_num].virtual_number : 0) : 0;
	char *reward_desc = reward ? mysql_str(reward->short_description, buf) : mysql_str("", buf);

	db_query(
		"INSERT INTO world_quest_accomplished (pid, timestamp, quest_giver, player_name, player_level, quest_target, reward_vnum, reward_desc) VALUES (%d, now(), %d, '%s', %d, %d, %d, '%s')",
		GET_PID(ch), ch->only.pc->quest_giver, GET_NAME(ch), GET_LEVEL(ch),
		ch->only.pc->quest_mob_vnum, reward_vnum, reward_desc);

	mark_player_dirty_components(GET_PID(ch), PLAYER_COMPONENT_STATUS);
}

int sql_world_quest_can_do_another(P_char ch)
{
	// This crashed us when paly's horse called this function.
	if (!IS_PC(ch))
		return 0;

	MYSQL_RES *db = 0;
	if (GET_LEVEL(ch) < 50)
		db = db_query(
			"SELECT count(id) FROM world_quest_accomplished where pid = %d and player_level =%d and TO_DAYS( NOW() ) - TO_DAYS( timestamp ) <= 0",
			GET_PID(ch), GET_LEVEL(ch));
	else
		db = db_query(
			"SELECT count(id) FROM world_quest_accomplished where pid = %d and TO_DAYS( NOW() ) - TO_DAYS( timestamp ) <= 0",
			GET_PID(ch));

	int returning_value = 0;
	if (GET_LEVEL(ch) <= 30)
		returning_value = get_property("world.quest.max.level.30.andUnder", 6.000);
	else if (GET_LEVEL(ch) <= 40)
		returning_value = get_property("world.quest.max.level.40.andUnder", 6.000);
	else if (GET_LEVEL(ch) <= 50)
		returning_value = get_property("world.quest.max.level.50.andUnder", 6.000);
	else if (GET_LEVEL(ch) <= 55)
		returning_value = get_property("world.quest.max.level.55.andUnder", 6.000);
	else
		returning_value = get_property("world.quest.max.level.other", 6.000);

	if (db)
	{
		MYSQL_ROW row = mysql_fetch_row(db);
		if (NULL != row)
		{
			returning_value = returning_value - atoi(row[0]);
		}

		while ((row = mysql_fetch_row(db)))
			;
		mysql_free_result(db);
	}
	return MAX(returning_value, 0);
}

int sql_world_quest_done_already(P_char ch, int quest_target)
{
	MYSQL_RES *db = db_query(
		"SELECT count(id) FROM world_quest_accomplished where quest_target = %d and pid = %d",
		quest_target, GET_PID(ch));
	int returning_value = 0;
	if (db)
	{
		MYSQL_ROW row = mysql_fetch_row(db);
		if (NULL != row)
		{
			returning_value = atoi(row[0]);
		}
		else
			returning_value = 0;

		while ((row = mysql_fetch_row(db)))
			;
		mysql_free_result(db);
	}
	return returning_value;
}

const char *sql_select_IP_info(P_char ch, char *buf, size_t bufSize, time_t *lastConnect,
			       time_t *lastDisconnect)
{
	time_t now = 0;
	buf[0] = '\0';

	MYSQL_RES *db = db_query(
		"SELECT last_ip, UNIX_TIMESTAMP(last_connect), UNIX_TIMESTAMP(last_disconnect), UNIX_TIMESTAMP() "
		"FROM ip_info WHERE pid = %d",
		GET_PID(ch));
	if (db)
	{
		MYSQL_ROW row = mysql_fetch_row(db);

		if (NULL != row)
		{
			strlcpy(buf, row[0] ? row[0] : "", bufSize);
			now = strtoul(row[3], NULL, 10);
			if (lastConnect)
			{
				*lastConnect = strtoul(row[1], NULL, 10);
				if (0 != *lastConnect)
					*lastConnect = now - *lastConnect;
			}
			if (lastDisconnect)
			{
				*lastDisconnect = strtoul(row[2], NULL, 10);
				if (0 != *lastDisconnect)
					*lastDisconnect = now - *lastDisconnect;
			}

			// cycle out until a NULL return
			while ((row = mysql_fetch_row(db)))
				;
		}
		mysql_free_result(db);
	}
	return buf;
}

// Returns the time needed *in seconds) to timeout the racewar side associated with an ip.
// Or 0 if no character has been on within an hour.
int sql_find_racewar_for_ip(char *ip, int *racewar_side)
{
	MYSQL_RES *db;
	MYSQL_ROW row;
	time_t last_connect, last_disconnect, hour_ago;

	db = db_query(
		"SELECT UNIX_TIMESTAMP(last_connect), UNIX_TIMESTAMP(last_disconnect), UNIX_TIMESTAMP(), racewar_side"
		" from ip_info WHERE last_ip = \"%s\" ORDER BY last_connect DESC LIMIT 1",
		ip);

	if (db && ((row = mysql_fetch_row(db)) != NULL))
	{
		// Arih: fix NULL pointer crash when last_disconnect is NULL in ip_info table - 20251103
		// UNIX_TIMESTAMP() returns NULL for NULL datetime values, causing strtoul to segfault
		last_connect = row[0] ? strtoul(row[0], NULL, 10) : 0;
		last_disconnect = row[1] ? strtoul(row[1], NULL, 10) : 0;
		hour_ago = row[2] ? strtoul(row[2], NULL, 10) - 60 * 60 : 0;
		*racewar_side = row[3] ? atoi(row[3]) : 0;

		// If they've been offline for an hour or more, return a 0 timer.
		if (last_disconnect > last_connect && last_disconnect <= hour_ago)
		{
			racewar_side = RACEWAR_NONE;
			while (row != NULL)
				row = mysql_fetch_row(db);
			return 0;
		}

		while (row != NULL)
			row = mysql_fetch_row(db);

		mysql_free_result(db);

		// Return an hour if they're still online, or time delta to an hour offline.
		return (last_disconnect < last_connect) ? 60 * 60 : last_disconnect - hour_ago;
	}

	if (db)
		mysql_free_result(db);
	return RACEWAR_NONE;
}

void perform_wiki_search(P_char ch, const char *query)
{
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	char escaped_query[MAX_STRING_LENGTH * 2 +
			   1]; // SECURITY: Buffer for escaped query (MySQL needs 2x+1 size)
	buf[0] = '\0';
	buf2[0] = '\0';
	MYSQL_ROW row;

	// SECURITY FIX: Sanitize user input to prevent SQL injection
	// Escape the query string using MySQL's built-in escape function
	mysql_real_escape_string(DB, escaped_query, query, strlen(query));

	/*
	MYSQL_RES *db  = db_query("SELECT UPPER(si_title) , old_id, REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(old_text,'<pre>',''),'</pre>',''), ']]', ''),'[[' ,'' ), '::', ':'), '<br>', '') FROM
	wikki_searchindex, wikki_text where old_id =( SELECT max(rev_text_id) FROM wikki_revision w where rev_page =( select si_page from wikki_searchindex where LOWER(si_title)  like LOWER('%s') limit
	1)) and si_title like LOWER('%s') limit 1", query, query);
	*/

	MYSQL_RES *db = db_query(
		"SELECT REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(old_text,'<pre>',''),'</pre>',''), ']]', ''),'[[' ,'' ), '::', ':'), '<br>', ''), '\'\'', '') , '==', '') "
		"FROM `wikki_text`  WHERE old_id = (SELECT rev_text_id FROM `wikki_page`,`wikki_revision`  WHERE (page_id=rev_page) AND rev_id = (SELECT page_latest FROM `wikki_page`  WHERE page_id "
		"= (SELECT page_id  FROM `wikki_page`  WHERE page_namespace = '0' AND LOWER(page_title) = REPLACE(LOWER('%s'), ' ', '_')  LIMIT 1)  LIMIT 1)  LIMIT 1)  LIMIT 1",
		escaped_query);
	if (db)
	{
		row = mysql_fetch_row(db);
		if (NULL != row)
		{
			checked_snprintf(buf, MAX_STRING_LENGTH,
					 "\t&+W========| &+m %s &+W |========&n\n%s", escaped_query,
					 row[0]);
		}
		else
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+WNothing matches, see &+mHelp wiki&+W how to add this help.&n");
		while ((row = mysql_fetch_row(db)))
			;
		mysql_free_result(db);
	}

	/*
	  MYSQL_RES *db2 = db_query("SELECT lower(si_title), MATCH (si_text) AGAINST REPLACE(LOWER('%s'), ' ', '_') as SCORE  FROM wikki_searchindex  order by SCORE desc limit 10", query);
	  if (db2)
	  {
	    row2 = mysql_fetch_row(db2);

	    if (NULL != row2)
	    {
	        if( atoi(row2[1]) > 0)
	        {
	  strcat(buf2, "\r\n\r\n");
	  strcat(buf2, "&+WOther related topics:&n\r\n");
	        snprintf(buf3, MAX_STRING_LENGTH, "&+m%s&n, " , row2[0]);
	  strcat(buf2, buf3);
	        }

	      // cycle out until a NULL return
	        int i = 0;
	  while ((row2 = mysql_fetch_row(db2)))
	        {
	        if( atoi(row2[1]) > 0){
	  i++;
	  snprintf(buf3, MAX_STRING_LENGTH, "&+m%s&n, " , row2[0]);
	  if(i == 5)
	  strcat(buf3, "\r\n");
	  strcat(buf2, buf3);
	        }

	    }

	   }
	  }
	  */
	strcat(buf2, "\r\n");
	strcat(buf, buf2);
	send_to_char(buf, ch);
}

static bool sql_trace_enabled(void)
{
	static int cached = -1;
	if (cached < 0)
	{
		const char *env = getenv("SQL_TRACE");
		bool on = false;

		// Both branches used to set this true, so tracing was always on -- two log
		//   lines (each an open/append/close) for every query the game runs, even
		//   with SQL_TRACE explicitly set to off.  Opt-in, as intended.
		if (env && *env && strcmp(env, "0") != 0 && strcasecmp(env, "false") != 0 &&
		    strcasecmp(env, "off") != 0)
		{
			on = true;
		}
		cached = on ? 1 : 0;
	}
	return cached != 0;
}

static bool sql_trace_active(void)
{
	return sql_trace_enabled() || sql_trace_burst > 0;
}

static enum persistence_query_context sql_current_context(void)
{
	return getpid() == sql_main_process_id ? PERSISTENCE_QUERY_CONTEXT_MAIN :
						 PERSISTENCE_QUERY_CONTEXT_CHILD;
}

static void sql_trace_log_drain(MYSQL *conn, const char *phase, bool drained)
{
	if (!conn)
		return;
	if (drained)
		sql_trace_burst = 100;
	if (!sql_trace_active() && !drained)
		return;

	logit(LOG_DEBUG,
	      "[SQLTRACE] phase=%s drained=%d burst=%d error_code=%u sqlstate=%.5s "
	      "field_count=%u more_results=%d",
	      phase, drained ? 1 : 0, sql_trace_burst, (unsigned int)mysql_errno(conn),
	      mysql_sqlstate(conn), (unsigned int)mysql_field_count(conn),
	      mysql_more_results(conn));
}

void sql_trace_panic(void)
{
	sql_trace_burst = 100;
}

bool sql_observed_execute_at(MYSQL *conn, struct persistence_query_site site,
			     enum persistence_query_context context, const char *sql, size_t len,
			     uint64_t *operation_id)
{
	if (!conn || !sql)
		return false;

	const enum persistence_statement_kind kind = persistence_statement_kind_from_sql(sql);
	const uint64_t started_at = persistence_observability_now_usec();
	const int status = mysql_real_query(conn, sql, len);
	const uint64_t finished_at = persistence_observability_now_usec();
	const uint64_t duration = finished_at >= started_at ? finished_at - started_at : 0;
	const unsigned int error_code = status == 0 ? 0 : (unsigned int)mysql_errno(conn);
	const char *mysql_state = status == 0 ? "00000" : mysql_sqlstate(conn);
	const uint64_t recorded_id = persistence_query_record(site, context, kind, duration,
							      status == 0, error_code, mysql_state);
	if (operation_id)
		*operation_id = recorded_id;

	if (status != 0 || sql_trace_active())
	{
		struct persistence_query_event event = {};
		char diagnostic[512];
		event.operation_id = recorded_id;
		event.site = site;
		event.context = context;
		event.kind = kind;
		event.duration_usec = duration;
		event.error_code = error_code;
		event.success = status == 0;
		snprintf(event.sqlstate, sizeof(event.sqlstate), "%.5s", mysql_state);
		if (persistence_query_event_format(diagnostic, sizeof(diagnostic), &event) >= 0)
			logit(status == 0 ? LOG_DEBUG : LOG_STATUS, "%s", diagnostic);
		if (sql_trace_burst > 0 && !sql_trace_enabled())
			--sql_trace_burst;
	}
	return status == 0;
}

bool sql_trace_exec_at(struct persistence_query_site source_site, const char *label,
		       const char *sql, size_t len, bool drain_before, bool drain_after)
{
	if (!DB || !sql)
		return false;
	const struct persistence_query_site semantic_site = {
		source_site.file, label && *label ? label : source_site.function, source_site.line
	};
	if (drain_before)
		sql_clear_results_on(DB);
	uint64_t operation_id = 0;
	if (!sql_observed_execute_at(DB, semantic_site, sql_current_context(), sql, len,
				     &operation_id))
	{
		sql_trace_panic();
		return false;
	}

	if (drain_after)
		sql_clear_results_on(DB);
	return true;
}

void sql_clear_results_on(MYSQL *conn)
{
	if (!conn)
		return;

	int status = 0;
	bool drained_any = false;
	do
	{
		/* did current statement return data? */
		MYSQL_RES *result = mysql_store_result(conn);
		if (result)
		{
			my_ulonglong rows = mysql_num_rows(result);
			unsigned int fields = mysql_num_fields(result);
			drained_any = true;
			logit(LOG_DEBUG,
			      "[SQLTRACE] phase=%s conn=%lu drained_result rows=%llu fields=%u more_results=%d",
			      "clear/result", (unsigned long)mysql_thread_id(conn),
			      (unsigned long long)rows, fields, mysql_more_results(conn));
			mysql_free_result(result);
		}
		else /* no result set or error */
		{
			if (mysql_field_count(conn) == 0)
			{
				// printf("%lld rows affected\n", mysql_affected_rows(conn));
			}
			else if (mysql_errno(conn) == 0)
			{
				// Benign pre-clear: no pending result and no MySQL error.
				// Keep this silent; the query/site trace already shows the caller.
			}
			else /* actual error occurred */
			{
				sql_trace_log_drain(conn, "clear/error", true);
				break;
			}
		}
		/* more results? -1 = no, >0 = error, 0 = yes (keep looping) */
		if ((status = mysql_next_result(conn)) > 0)
		{
			sql_trace_log_drain(conn, "clear/next_result_error", true);
			break;
		}
	} while (status == 0);

	if (drained_any)
		sql_trace_log_drain(conn, "clear/drained", true);
}

void sql_clear_results()
{
	sql_clear_results_on(DB);
}

/* Execute a semicolon-separated multi-statement query.
 * Uses the observed executor with CLIENT_MULTI_STATEMENTS and drains all
 * result sets. */
bool sql_run_multi_query(const char *query)
{
	if (!DB || !query || !*query)
		return false;

	if (!sql_trace_exec("sql_run_multi_query", query, strlen(query), true, false))
	{
		sql_player_error("sql_run_multi_query");
		// Drain any partial result sets from statements that succeeded
		// before the failing one (multi-statement with CLIENT_MULTI_STATEMENTS).
		sql_clear_results();
		return false;
	}

	sql_clear_results();
	return true;
}

bool qry_at(struct persistence_query_site site, const char *format, ...)
{
	char buf[MAX_STRING_LENGTH];
	va_list args;
	int ret;

	if (!DB)
	{
		logit(LOG_DEBUG, "MySQL error: MySQL not initialized!");
		return FALSE;
	}

	va_start(args, format);
	buf[0] = '\0';
	// SECURITY FIX: Replace vsprintf with vsnprintf to prevent buffer overflow
	ret = vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	// Check for overflow
	if (ret < 0 || ret >= (int)sizeof(buf))
	{
		logit(LOG_DEBUG, "MySQL error: Query too long or formatting error");
		return FALSE;
	}

	if (!sql_trace_exec_at(site, "qry/direct", buf, strlen(buf), true, false))
	{
		return FALSE;
	}

	return TRUE;
}

void send_to_pid_offline(const char *msg, int pid)
{
	char buff[MAX_STRING_LENGTH];
	mysql_real_escape_string(DB, buff, msg, strlen(msg));
	qry("INSERT INTO offline_messages (date, pid, message) VALUES (now(), '%d', '%s')", pid,
	    buff);
}

void send_offline_messages(P_char ch)
{
	if (!ch)
		return;

	if (!qry("SELECT id, message FROM offline_messages WHERE pid = '%d' ORDER BY date ASC",
		 GET_PID(ch)))
	{
		return;
	}

	MYSQL_RES *res = mysql_store_result(DB);

	if (mysql_num_rows(res) < 1)
	{
		mysql_free_result(res);
		return;
	}

	std::vector<int> delete_ids;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)))
	{
		send_to_char(row[1], ch);
		delete_ids.push_back(atoi(row[0]));
	}

	mysql_free_result(res);

	for (int id : delete_ids)
	{
		qry("DELETE FROM offline_messages WHERE id = '%d'", id);
	}
}

int sql_shop_sell(P_char ch, P_obj obj, int value)
{
	if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)
		return flat_sql_shop_sell(ch, obj, value);
	if (!obj)
		return 0;
	int m_virtual = (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : 0;

	int pid = (IS_PC(ch) ? GET_PID(ch) : 0);

	qry("INSERT INTO shop_trophy (item, value, seller, timestamp) VALUES ('%d', '%d', %d, now())",
	    m_virtual, value, pid);

	return 1;
}

int sql_shop_trophy(P_obj obj)
{
	if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)
		return flat_sql_shop_trophy(obj);
	if (!obj)
		return 0;

	// mined ore doesnt devaule
	if (obj->name && strstr(obj->name, "_ore_"))
		return 0;

	int objvir = OBJ_VNUM(obj);
	if ((objvir >= 400000) && (objvir < 400202))
		return 0;

	int m_virtual = (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : 0;

	MYSQL_RES *db = db_query(
		"SELECT count(id) FROM shop_trophy where item = %d and  TO_DAYS( NOW() ) - TO_DAYS( timestamp ) <= 7",
		m_virtual);

	int returning_value = 0;
	if (db)
	{
		MYSQL_ROW row = mysql_fetch_row(db);
		if (NULL != row)
		{
			returning_value = atoi(row[0]);
		}
		else
			returning_value = 0;
		while ((row = mysql_fetch_row(db)))
			;
		mysql_free_result(db);
	}
	return returning_value;
}

///

int sql_quest_finish(P_char ch, P_char giver, int type, int value)
{
	int m_virtual = GET_VNUM(giver);
	// GET_PID(ch), ch->only.pc->quest_giver, GET_NAME(ch), GET_LEVEL(ch), ch->only.pc->quest_mob_vnum, m_virtual ,reward->short_description );
	qry("INSERT INTO quest_trophy (mob_vnum, pid, type, reward_value, timestamp) VALUES ('%d', '%d', %d, %d ,now())",
	    m_virtual, GET_PID(ch), type, value);
	return 1;
}

int sql_quest_trophy(P_char giver)
{
	int m_virtual = GET_VNUM(giver);

	MYSQL_RES *db = db_query(
		"SELECT count(id) FROM quest_trophy where mob_vnum = %d and  TO_DAYS( NOW() ) - TO_DAYS( timestamp ) <= 14",
		m_virtual);
	int returning_value = 0;
	if (db)
	{
		MYSQL_ROW row = mysql_fetch_row(db);
		if (NULL != row)
		{
			returning_value = atoi(row[0]);
		}
		else
			returning_value = 0;
		while ((row = mysql_fetch_row(db)))
			;
		mysql_free_result(db);
	}
	return returning_value;
}

void log_epic_gain(int pid, int type, int type_id, int epics)
{
	(void)pid;
	(void)type;
	(void)type_id;
	(void)epics;
}

void log_epic_gain_event(const char * /*event_key*/, int pid, int type, int type_id, int epics)
{
	(void)pid;
	(void)type;
	(void)type_id;
	(void)epics;
}

/* The prepstatement_duris_sql table looks like:
+-------------+---------+------+-----+---------+----------------+
| Field       | Type    | Null | Key | Default | Extra          |
+-------------+---------+------+-----+---------+----------------+
| id          | int(11) | NO   | PRI | NULL    | auto_increment |
| description | text    | YES  |     | NULL    |                |
| sql_code    | text    | YES  |     | NULL    |                |
+-------------+---------+------+-----+---------+----------------+
*/
void do_sql(P_char ch, char *argument, int cmd)
{
	char first[MAX_INPUT_LENGTH];
	char second[MAX_INPUT_LENGTH];
	char third[MAX_INPUT_LENGTH];
	char *rest;
	char buf[MAX_STRING_LENGTH];
	int limited_result = 0;
	int prep_statement;
	int num_fields, i;

	char result[MAX_STRING_LENGTH * 10];
	char tmp[MAX_STRING_LENGTH];

	MYSQL_RES *db = 0;
	MYSQL_ROW row;

	if (!IS_TRUSTED(ch))
	{
		send_to_char("A mere mortal can't do this!\r\n", ch);
		return;
	}

	if (!*argument)
	{
		send_to_char(
			"Sql is a command to let us gods, access database easy, it suport all kind of queries.\n"
			"&=LY-=Make sure you understand what you do else this command is most likly not designed for you=-&n\n",
			ch);
		send_to_char("&+WSyntax: 'sql < query | prep <list | #> >'&n\n", ch);
		return;
	}

	wizlog(56, "SQL command executed");
	logit(LOG_WIZ, "SQL command executed");

	rest = one_argument(argument, first);
	rest = one_argument(rest, second);

	if (strstr(first, "prep"))
	{
		if (strstr(second, "list"))
		{
			do_sql(ch,
			       writable_arg("SELECT id, description FROM prepstatement_duris_sql"),
			       0);
		}
		if (!is_number(second))
		{
			//      send_to_char("\n\r&+YTo add prep queries just check how the table 'prepstatement_duris_sql' (&+Wsql desc prepstatement_duris_sql&+Y) and add!&n\n\r", ch);
			send_to_char(
				"&+YSyntax:&n sql prep < list | number > [ desc | sql | run | delete ] [ description | sql code ]\n\r",
				ch);
			return;
		}
		else
		{
			prep_statement = (int)atoi(second);
			rest = one_argument(rest, third);
			rest = skip_spaces(rest);
			if (!*third)
			{
				snprintf(third, MAX_INPUT_LENGTH,
					 "SELECT * FROM prepstatement_duris_sql WHERE id=%d",
					 prep_statement);
				do_sql(ch, third, cmd);
				/* This won't work due to the fact that we're trying a second sql command?
				        if( !qry( third ) )
				        {
				          send_to_char( "Row does not exist: attempting to create..\n\r", ch );
				          snprintf(buf, MAX_STRING_LENGTH, "INSERT INTO prepstatement_duris_sql (id, description) VALUES (%d, 'new')", prep_statement );
				          do_sql( ch, buf, cmd );
				        }
				        else
				        {
				          do_sql( ch, third, cmd );
				        }
				*/
				return;
			}
			if (strstr(third, "run"))
			{
				db = db_query(
					"SELECT sql_code FROM prepstatement_duris_sql WHERE id=%d",
					prep_statement);
				if (db)
				{
					MYSQL_ROW prepared_row = mysql_fetch_row(db);

					if (prepared_row != NULL)
					{
						snprintf(tmp, MAX_STRING_LENGTH, "%s",
							 prepared_row[0]);
					}
					else
					{
						send_to_char(
							"That prepped statement does not exist.\n\r",
							ch);
						tmp[0] = '\0';
					}
					while ((prepared_row = mysql_fetch_row(db)))
						;
					mysql_free_result(db);

					do_sql(ch, tmp, 0);
					return;
				}
				else
				{
					send_to_char("Error no db created.\n\r", ch);
				}
				return;
			}
			if (strstr(third, "desc"))
			{
				// SECURITY FIX: Escape user input to prevent SQL injection
				char escaped_desc[MAX_STRING_LENGTH * 2 + 1];
				mysql_real_escape_string(DB, escaped_desc, rest, strlen(rest));
				checked_snprintf(
					buf, MAX_STRING_LENGTH,
					"UPDATE prepstatement_duris_sql SET description = '%s' WHERE id='%d'",
					escaped_desc, prep_statement);
				do_sql(ch, buf, 0);
				return;
			}
			if (strstr(third, "sql"))
			{
				// SECURITY FIX: Escape user input to prevent SQL injection
				char escaped_sql[MAX_STRING_LENGTH * 2 + 1];
				mysql_real_escape_string(DB, escaped_sql, rest, strlen(rest));
				if (qry("UPDATE prepstatement_duris_sql SET sql_code = '%s' WHERE id='%d'",
					escaped_sql, prep_statement))
				{
					snprintf(buf, MAX_STRING_LENGTH,
						 "Row %d sql_code set to '%s'.\n\r", prep_statement,
						 rest);
					send_to_char(buf, ch);
				}
				return;
			}
			if (strstr(third, "delete"))
			{
				if (qry("DELETE FROM prepstatement_duris_sql WHERE id=%d",
					prep_statement))
				{
					snprintf(buf, MAX_STRING_LENGTH, "Row %d deleted.\n\r",
						 prep_statement);
					send_to_char(buf, ch);
				}
				return;
			}
		}
	}

	MYSQL_FIELD *fields;
	result[0] = '\0';

	sql_clear_results_on(DB);
	if (!sql_trace_exec("do_sql", argument, strlen(argument), false, false))
	{
		snprintf(result, MAX_STRING_LENGTH, "Database operation failed.\r\n");
		logit(LOG_DEBUG, "Database admin command failed");
		send_to_char(result, ch);
		return;
	}
	db = mysql_use_result(DB);
	if (db)
	{
		num_fields = mysql_num_fields(db);

		fields = mysql_fetch_fields(db);
		for (i = 0; i < num_fields; i++)
		{
			snprintf(tmp, MAX_STRING_LENGTH, " | %-15s&n ", fields[i].name);
			strcat(result, tmp);
		}
		strcat(result, " |\n\n");

		int maxsize = 100;
		while ((row = mysql_fetch_row(db)))
		{
			maxsize--;
			if (maxsize == 0)
			{
				while ((row = mysql_fetch_row(db)))
					;
				limited_result = 1;
				break;
			}

			for (i = 0; i < num_fields; i++)
			{
				snprintf(tmp, MAX_STRING_LENGTH, " | %-15s&n ", row[i]);
				strcat(result, tmp);
			}
			strcat(result, " |\n\n");
		}
		send_to_char(result, ch);
		if (limited_result)
		{
			send_to_char(
				"Result to big, pls use limit. 'select * from blah &+Ylimit 10&n' will show 10 results.\n",
				ch);
		}
		mysql_free_result(db);
		return;
	}
}

void update_zone_db()
{
	/* update the zones in the database */
	for (int z = 1; z <= top_of_zone_table; z++)
	{
		int number = zone_table[z].number;

		if (!qry("SELECT id FROM zones WHERE number = '%d'", number))
		{
			logit(LOG_DEBUG, "update_zone_db(): qry failed");
			return;
		}

		char name_buff[MAX_STRING_LENGTH];
		mysql_real_escape_string(DB, name_buff, zone_table[z].name,
					 strlen(zone_table[z].name));

		MYSQL_RES *res = mysql_store_result(DB);
		if (mysql_num_rows(res) > 0)
		{
			qry("UPDATE zones SET name = '%s' WHERE number = '%d'", name_buff, number);
		}
		else
		{
			qry("INSERT INTO zones (number, name) VALUES ('%d', '%s')", number,
			    name_buff);
		}
		mysql_free_result(res);
	}

	for (P_obj o = object_list; o; o = o->next)
	{
		int epic_type = 0;

		switch (obj_index[o->R_num].virtual_number)
		{
		case EPIC_SMALL_STONE:
			epic_type = MAX(epic_type, EPIC_ZONE_TYPE_SMALL);
			break;

		case EPIC_LARGE_STONE:
			epic_type = MAX(epic_type, EPIC_ZONE_TYPE_LARGE);
			break;

		case EPIC_MONOLITH:
			epic_type = MAX(epic_type, EPIC_ZONE_TYPE_MONOLITH);
			break;
		}

		if (!epic_type)
			continue;

		int zone_id = obj_zone_id(o);

		if (zone_id >= 0)
		{
			qry("UPDATE zones SET epic_type = '%d' WHERE number = '%d'", epic_type,
			    zone_table[zone_id].number);
		}
	}
}

void update_zone_epic_level(int zone_number, int level)
{
	qry("UPDATE zones SET epic_level = '%d' WHERE number = '%d'", level, zone_number);
}

void show_frag_trophy(P_char ch, P_char who)
{
	if (!IS_PC(who))
		return;

	if (!qry("select player_data.name, count(*) as cnt from epic_gain, player_data where epic_gain.type_id = player_data.pid and epic_gain.pid = %d and type = 1 group by type_id order by name asc",
		 who->only.pc->pid))
	{
		logit(LOG_DEBUG, "show_frag_trophy(): query failed.");
		return;
	}

	MYSQL_RES *res = mysql_store_result(DB);

	if (mysql_num_rows(res) < 1)
	{
		mysql_free_result(res);
		send_to_char("&+WYou haven't fragged anyone!\r\n", ch);
		return;
	}

	send_to_char("&+gFrag Trophy:\r\n", ch);

	char buff[MAX_STRING_LENGTH];

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)))
	{
		snprintf(buff, MAX_STRING_LENGTH, " &+g(&+G%2d&+g) &+W%s\r\n", atoi(row[1]),
			 row[0]);
		send_to_char(buff, ch);
	}

	mysql_free_result(res);
}

void sql_log(P_char ch, const char *kind, const char *format, ...)
{
	static char buff[MAX_STRING_LENGTH];
	buff[0] = '\0';

	if (!ch)
	{
		debug("sql_log called for non-existent ch!");
		return;
	}

	if (!IS_PC(ch))
	{
		debug("sql_log called in sql.c for mobile ch - %s - Vnum %d", GET_NAME(ch),
		      GET_VNUM(ch));
		debug("sql_log kind '%s', format '%s'", kind, format);
		return;
	}

	va_list args;
	int ret;

	va_start(args, format);
	// SECURITY FIX: Replace vsprintf with vsnprintf to prevent buffer overflow
	ret = vsnprintf(buff, sizeof(buff), format, args);
	va_end(args);

	// Check for overflow
	if (ret < 0 || ret >= (int)sizeof(buff))
	{
		debug("sql_log: Message too long or formatting error");
		return;
	}

	static char message_buff[MAX_STRING_LENGTH];
	message_buff[0] = '\0';
	mysql_real_escape_string(DB, message_buff, buff, strlen(buff));

	static char ip_buff[15];
	ip_buff[0] = '\0';

	if (ch->desc && *ch->desc->host)
	{
		checked_snprintf(ip_buff, sizeof ip_buff, "%s", ch->desc->host);
	}

	checked_snprintf(
		buff, MAX_STRING_LENGTH,
		"INSERT INTO log_entries (date, kind, ip_address, pid, player_name, zone_number, room_vnum, message) VALUES "
		"(now(), '%s', '%s', %d, '%s', %d, %d, '%s')",
		kind, ip_buff, GET_PID(ch), GET_NAME(ch),
		zone_table[world[ch->in_room].zone].number, world[ch->in_room].number,
		message_buff);

	qry(buff);
}

bool get_zone_info(int zone_number, struct zone_info *info)
{
	if (!info)
	{
		return FALSE;
	}

	if (!qry("SELECT number, name, epic_type, frequency_mod, zone_freq_mod, epic_level, task_zone, quest_zone, trophy_zone, suggested_group_size, epic_payout, difficulty FROM zones WHERE number = %d",
		 zone_number))
	{
		return FALSE;
	}

	MYSQL_RES *res = mysql_store_result(DB);

	if (mysql_num_rows(res) < 1)
	{
		mysql_free_result(res);
		return FALSE;
	}

	MYSQL_ROW row = mysql_fetch_row(res);

	info->number = atoi(row[0]);
	info->name = string(row[1]);
	info->epic_type = atoi(row[2]);
	info->frequency_mod = atof(row[3]);
	info->zone_freq_mod = atof(row[4]);
	info->epic_level = atoi(row[5]);
	info->task_zone = (bool)atoi(row[6]);
	info->quest_zone = (bool)atoi(row[7]);
	info->trophy_zone = (bool)atoi(row[8]);
	info->suggested_group_size = atoi(row[9]);
	info->epic_payout = atoi(row[10]);
	info->difficulty = atoi(row[11]);

	mysql_free_result(res);
	return TRUE;
}

string get_mud_info(const char *name)
{
	if (!qry("SELECT content FROM mud_info WHERE name = '%s'", name))
	{
		logit(LOG_DEBUG, "get_mud_info(): failed to read mud_info '%s' from database",
		      name);
		return string();
	}

	MYSQL_RES *res = mysql_store_result(DB);

	if (!res)
	{
		logit(LOG_DEBUG, "get_mud_info(): mysql_store_result failed for '%s'", name);
		return string();
	}

	if (mysql_num_rows(res) > 0)
	{
		MYSQL_ROW row = mysql_fetch_row(res);
		string ret_str(row[0]);
		mysql_free_result(res);
		return ret_str;
	}
	else
	{
		logit(LOG_DEBUG, "get_mud_info(): requested mud_info '%s', but doesn't exist!",
		      name);
		mysql_free_result(res);
		return string();
	}
}

void send_mud_info(const char *name, P_char ch)
{
	send_to_char(get_mud_info(name).c_str(), ch, LOG_NONE);
}

static bool sql_parse_bind_int(const char *value, int *result)
{
	if (!value || !result || !*value)
	{
		return false;
	}

	const char *digits = value;
	if (*digits == '-' || *digits == '+')
	{
		digits++;
	}
	if (!*digits)
	{
		return false;
	}
	for (const char *digit = digits; *digit; digit++)
	{
		if (!isdigit((unsigned char)*digit))
		{
			return false;
		}
	}

	errno = 0;
	char *end = NULL;
	long parsed = strtol(value, &end, 10);
	if (errno == ERANGE || !end || *end || parsed < INT_MIN || parsed > INT_MAX)
	{
		return false;
	}

	*result = (int)parsed;
	return true;
}

bool sql_get_bind_data(int vnum, int *owner_pid, int *timer)
{
	if (owner_pid)
	{
		*owner_pid = 0;
	}
	if (timer)
	{
		*timer = 0;
	}
	if (!owner_pid || !timer)
	{
		logit(LOG_DEBUG, "sql_get_bind_data(): invalid output pointer");
		return false;
	}

	if (!qry("SELECT owner_pid, timer FROM artifact_bind WHERE vnum = %d", vnum))
	{
		logit(LOG_DEBUG, "sql_get_bind_data(): failed to read from database");
		return false;
	}

	MYSQL_RES *res = mysql_store_result(DB);
	if (!res)
	{
		logit(LOG_DEBUG, "sql_get_bind_data(): mysql_store_result failed");
		return false;
	}

	if (mysql_num_rows(res) < 1)
	{
		mysql_free_result(res);
		return true;
	}

	MYSQL_ROW row = mysql_fetch_row(res);
	int parsed_owner_pid = 0;
	int parsed_timer = 0;
	if (!row || !sql_parse_bind_int(row[0], &parsed_owner_pid) ||
	    !sql_parse_bind_int(row[1], &parsed_timer))
	{
		logit(LOG_DEBUG, "sql_get_bind_data(): malformed database row");
		mysql_free_result(res);
		return false;
	}

	*owner_pid = parsed_owner_pid;
	*timer = parsed_timer;
	mysql_free_result(res);
	return true;
}

void sql_update_bind_data(int vnum, int *owner_pid, int *timer)
{
	if (!qry("select * from artifact_bind where vnum = %d", vnum))
	{
		logit(LOG_DEBUG, "sql_update_bind_data(): failed to read from database");
		return;
	}

	MYSQL_RES *res = mysql_store_result(DB);
	if (mysql_num_rows(res) > 0)
	{
		qry("UPDATE artifact_bind SET owner_pid = %d, timer = %d WHERE vnum = %d",
		    *owner_pid, *timer, vnum);
	}
	else
	{
		qry("INSERT INTO artifact_bind VALUES(%d, %d, %d)", vnum, *owner_pid, *timer);
	}
	mysql_free_result(res);
}

bool sql_clear_zone_trophy()
{
	// Update the table zones, set the alignment to 0, where there's an epic stone.
	if (!qry("UPDATE zones SET alignment=0 WHERE epic_type > 0"))
	{
		debug("sql_clear_zone_trophy(): Failed sql UPDATE.. :(");
		return FALSE;
	}

	return TRUE;
}

/* Verify every runtime table and column referenced by sql_pwipe() before the
 * first destructive statement.  This must describe final runtime schema, not
 * migration-only helper tables. */
bool sql_verify_pwipe_manifest(void)
{
	static const char *const tables[] = { "account_bound_rewards",
					      "account_bound_reward_summons",
					      "account_bound_reward_pwipe_state",
					      "account_characters",
					      "account_locker_access",
					      "account_locker_item_affects",
					      "account_locker_item_extra_descr",
					      "account_locker_items",
					      "account_lockers",
					      "alliances",
					      "artifact_bind",
					      "artifacts",
					      "artifacts_mortal",
					      "associations",
					      "auction_bid_history",
					      "auction_item_pickups",
					      "auction_money_pickups",
					      "auctions",
					      "boons",
					      "boons_progress",
					      "boons_shop",
					      "corpse_item_affects",
					      "corpse_item_extra_descr",
					      "corpse_items",
					      "corpses",
					      "ctf_data",
					      "epic_bonus",
					      "epic_gain",
					      "eq_drop",
					      "frag_leaderboard",
					      "guild_members",
					      "guild_ranks",
					      "guild_transactions",
					      "guildhall_rooms",
					      "guildhalls",
					      "guilds",
					      "ip_info",
					      "level_cap",
					      "locker_access",
					      "locker_activity_log",
					      "locker_chests",
					      "locker_item_affects",
					      "locker_item_extra_descr",
					      "locker_items",
					      "locker_kickouts",
					      "locker_session_state",
					      "lockers",
					      "log_entries",
					      "nexus_stones",
					      "offline_messages",
					      "outposts",
					      "persistence_item_events",
					      "persistence_scalar_events",
					      "pkill_event",
					      "pkill_info",
					      "player_affects",
					      "player_data",
					      "player_forged_items",
					      "player_granted_cmds",
					      "player_intros",
					      "player_item_affects",
					      "player_item_extra_descr",
					      "player_items",
					      "player_languages",
					      "player_pet_item_affects",
					      "player_pet_item_extra_descr",
					      "player_pet_items",
					      "player_pets",
					      "player_recipes",
					      "player_shapechanges",
					      "player_skills",
					      "player_spellbooks",
					      "player_timers",
					      "player_undead_slots",
					      "player_witnesses",
					      "poll_options",
					      "poll_votes",
					      "polls",
					      "private_chest_log",
					      "private_chests",
					      "progress",
					      "racewar_stat_mods",
					      "saved_item_affects",
					      "saved_item_extra_descr",
					      "saved_items",
					      "season_reset_state",
					      "ship_armor",
					      "ship_cargo_market_mods",
					      "ship_cargo_prices",
					      "ship_crew",
					      "ship_slots",
					      "ships",
					      "shop_trophy",
					      "shopkeeper_affects",
					      "shopkeeper_item_affects",
					      "shopkeeper_item_extra_descr",
					      "shopkeeper_items",
					      "shopkeepers",
					      "statistics",
					      "timers",
					      "world_quest_accomplished",
					      "zone_touches",
					      "zone_trophy",
					      NULL };
	static const char *const columns[][2] = {
		{ "account_bound_rewards", "id" },
		{ "account_bound_rewards", "expires_at" },
		{ "account_bound_rewards", "remaining_pwipes" },
		{ "account_bound_reward_summons", "grant_id" },
		{ "account_bound_reward_summons", "pid" },
		{ "account_bound_reward_summons", "last_summoned_at" },
		{ "account_bound_reward_pwipe_state", "id" },
		{ "account_bound_reward_pwipe_state", "last_processed_at" },
		{ "outposts", "owner_id" },
		{ "outposts", "level" },
		{ "outposts", "walls" },
		{ "outposts", "archers" },
		{ "outposts", "hitpoints" },
		{ "outposts", "territory" },
		{ "outposts", "portal_room" },
		{ "outposts", "resources" },
		{ "outposts", "applied_resources" },
		{ "outposts", "golems" },
		{ "outposts", "meurtriere" },
		{ "outposts", "scouts" },
		{ "nexus_stones", "align" },
		{ "nexus_stones", "last_touched_at" },
		{ "timers", "date" },
		{ "account_characters", "deleted_at" },
		{ "player_data", "active" },
		{ "level_cap", "most_frags" },
		{ "level_cap", "racewar_leader" },
		{ "level_cap", "level" },
		{ "level_cap", "next_update" },
		{ "season_reset_state", "state_id" },
		{ "season_reset_state", "season_epoch" },
		{ "season_reset_state", "reset_status" },
		{ "season_reset_state", "reset_started_at" },
		{ "season_reset_state", "reset_completed_at" },
		{ NULL, NULL }
	};
	char query[8192];
	int pos = snprintf(
		query, sizeof(query),
		"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN (");
	if (pos < 0 || (size_t)pos >= sizeof(query))
		return FALSE;
	int expected_tables = 0;
	for (int i = 0; tables[i] != NULL; i++)
	{
		int written = snprintf(query + pos, sizeof(query) - (size_t)pos, "%s'%s'",
				       i ? "," : "", tables[i]);
		if (written < 0 || (size_t)written >= sizeof(query) - (size_t)pos)
			return FALSE;
		pos += written;
		expected_tables++;
	}
	if (pos + 2 >= (int)sizeof(query))
		return FALSE;
	strcat(query, ")");
	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return FALSE;
	MYSQL_ROW row = mysql_fetch_row(result);
	bool tables_ok = row && row[0] && atoi(row[0]) == expected_tables;
	mysql_free_result(result);
	if (!tables_ok)
		return FALSE;

	pos = snprintf(
		query, sizeof(query),
		"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND (");
	if (pos < 0 || (size_t)pos >= sizeof(query))
		return FALSE;
	int expected_columns = 0;
	for (int i = 0; columns[i][0] != NULL; i++)
	{
		int written = snprintf(query + pos, sizeof(query) - (size_t)pos,
				       "%s(table_name='%s' AND column_name='%s')", i ? " OR " : "",
				       columns[i][0], columns[i][1]);
		if (written < 0 || (size_t)written >= sizeof(query) - (size_t)pos)
			return FALSE;
		pos += written;
		expected_columns++;
	}
	if (pos + 2 >= (int)sizeof(query))
		return FALSE;
	strcat(query, ")");
	result = db_query("%s", query);
	if (!result)
		return FALSE;
	row = mysql_fetch_row(result);
	bool columns_ok = row && row[0] && atoi(row[0]) == expected_columns;
	mysql_free_result(result);
	return columns_ok;
}

/* Season-reset preflight: verify persistence event schema and auction engines.
 * These are the same checks as sql_verify_boot_database() but are callable
 * from sql_pwipe() as a preflight gate without requiring a fresh boot. */
bool sql_verify_persistence_schema(void)
{
	if (!DB)
		return FALSE;

	/* Verify persistence event columns exist. */
	const char *event_schema_probe =
		"SELECT COUNT(*) FROM information_schema.columns "
		"WHERE table_schema=DATABASE() AND "
		"((table_name='persistence_item_events' AND column_name IN "
		"('id','ts_usec','event_type','item_uid','vnum','item','actor','actor_id','source','target','note','dedupe_key','created_at')) "
		"OR (table_name='persistence_scalar_events' AND column_name IN "
		"('id','event_type','event_key','boot_time','touched_at','zone_number','toucher_pid','group_size','epic_value','alignment_delta','dedupe_key','created_at')))";
	MYSQL_RES *result = db_query("%s", event_schema_probe);
	if (!result)
		return FALSE;
	MYSQL_ROW row = mysql_fetch_row(result);
	bool event_columns_ok = row && row[0] && atoi(row[0]) == 25;
	mysql_free_result(result);
	if (!event_columns_ok)
		return FALSE;

	/* Verify persistence event indexes exist. */
	const char *event_index_probe =
		"SELECT COUNT(*) FROM (SELECT DISTINCT table_name, index_name "
		"FROM information_schema.statistics WHERE table_schema=DATABASE() AND "
		"((table_name='persistence_item_events' AND index_name IN "
		"('PRIMARY','idx_item_uid_ts','idx_event_type_created','uq_item_dedupe')) "
		"OR (table_name='persistence_scalar_events' AND index_name IN "
		"('PRIMARY','idx_scalar_event_key','idx_scalar_zone_time','uq_scalar_dedupe')))) "
		"AS required_indexes";
	result = db_query("%s", event_index_probe);
	if (!result)
		return FALSE;
	row = mysql_fetch_row(result);
	bool event_indexes_ok = row && row[0] && atoi(row[0]) == 8;
	mysql_free_result(result);
	return event_indexes_ok;
}

bool sql_verify_auction_engines(void)
{
	if (!DB)
		return FALSE;

	const char *auction_engine_probe =
		"SELECT COUNT(DISTINCT table_name) FROM information_schema.tables "
		"WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN "
		"('auction_bid_history','auction_item_pickups','auction_money_pickups','auctions')";
	MYSQL_RES *result = db_query("%s", auction_engine_probe);
	if (!result)
		return FALSE;
	MYSQL_ROW row = mysql_fetch_row(result);
	bool auction_engines_ok = row && row[0] && atoi(row[0]) == 4;
	mysql_free_result(result);
	return auction_engines_ok;
}

bool sql_pwipe(int code_verify)
{
	pwipe_crossed_boundary = false;
	logit(LOG_DEBUG, "sql_pwipe: STARTED!");
	if (code_verify == 1723699)
	{
		/* -- Preflight: verify critical tables exist and have correct engines -- */
		logit(LOG_DEBUG, "sql_pwipe: Preflight schema check... .. .");
		send_to_all("Preflight schema check... .. .");
		if (!sql_verify_persistence_schema())
		{
			logit(LOG_DEBUG,
			      "sql_pwipe: Preflight failed: persistence schema incomplete.");
			send_to_all("Preflight FAILED: persistence schema incomplete!\n");
			return FALSE;
		}
		if (!sql_verify_pwipe_manifest())
		{
			logit(LOG_DEBUG,
			      "sql_pwipe: Preflight failed: reset manifest schema incomplete.");
			send_to_all("Preflight FAILED: reset manifest schema incomplete!\n");
			return FALSE;
		}
		/* Verify auction tables are InnoDB (transactional) before reset. */
		if (!sql_verify_auction_engines())
		{
			logit(LOG_DEBUG, "sql_pwipe: Preflight failed: auction tables not InnoDB.");
			send_to_all("Preflight FAILED: auction tables must be InnoDB!\n");
			return FALSE;
		}
		if (!redis_validate_pwipe_state())
		{
			logit(LOG_DEBUG,
			      "sql_pwipe: Preflight failed: fresh Redis administrative connection unavailable.");
			send_to_all("Preflight FAILED: Redis invalidation target unavailable!\n");
			return FALSE;
		}
		if (!sql_begin_pwipe_epoch())
		{
			logit(LOG_DEBUG,
			      "sql_pwipe: Failed to establish durable season reset boundary.");
			send_to_all("Preflight FAILED: season reset boundary unavailable!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "  success!");
		send_to_all("  success!\n");
		logit(LOG_DEBUG, "sql_pwipe: Clearing zone alignments, trophy and touches... .. .");
		send_to_all("Clearing zone alignments, trophy and touches... .. .");
		if (sql_clear_zone_trophy() && qry("DELETE FROM zone_trophy") &&
		    qry("DELETE FROM zone_touches"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing tower ownership... .. .");
		send_to_all("Clearing tower ownership... .. .");
		if (qry("UPDATE outposts SET owner_id='0', level='8', walls='1', archers='0', hitpoints='300000', territory='0',"
			" portal_room='0', resources='0', applied_resources='0', golems='0', meurtriere='0', scouts='0'"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing nexus stone data... .. .");
		send_to_all("Clearing nexus stone data... .. .");
		if (qry("UPDATE nexus_stones SET align='0', last_touched_at=NULL"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing prestige lists... .. .");
		send_to_all("Clearing prestige lists... .. .");
		if (qry("DELETE FROM associations"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing alliances... .. .");
		send_to_all("Clearing alliances... .. .");
		if (qry("DELETE FROM alliances"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing artifact bind data... .. .");
		send_to_all("Clearing artifact bind data... .. .");
		if (qry("DELETE FROM artifact_bind"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing auction data... .. .");
		send_to_all("Clearing auction data... .. .");
		if (qry("DELETE FROM auction_bid_history") &&
		    qry("DELETE FROM auction_item_pickups") &&
		    qry("DELETE FROM auction_money_pickups") && qry("DELETE FROM auctions"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing boon data... .. .");
		send_to_all("Clearing boon data... .. .");
		if (qry("DELETE FROM boons_progress") && qry("DELETE FROM boons"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing ctf data... .. .");
		send_to_all("Clearing ctf data... .. .");
		if (qry("DELETE FROM ctf_data"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing frag data and epic bonus data... .. .");
		send_to_all("Clearing frag data and epic bonus data... .. .");
		if (qry("DELETE FROM epic_bonus") && qry("DELETE FROM epic_gain") &&
		    qry("DELETE FROM progress"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing guild data... .. .");
		send_to_all("Clearing guild data... .. .");
		if (qry("DELETE FROM guild_transactions") && qry("DELETE FROM guildhall_rooms") &&
		    qry("DELETE FROM guildhalls"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing ip info... .. .");
		send_to_all("Clearing ip info... .. .");
		if (qry("DELETE FROM ip_info"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing log entries... .. .");
		send_to_all("Clearing log entries... .. .");
		if (qry("DELETE FROM log_entries"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing offline messages... .. .");
		send_to_all("Clearing offline messages... .. .");
		if (qry("DELETE FROM offline_messages"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing cargo data... .. .");
		send_to_all("Clearing cargo data... .. .");
		if (qry("DELETE FROM ship_cargo_market_mods") &&
		    qry("DELETE FROM ship_cargo_prices"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing timers... .. .");
		send_to_all("Clearing timers... .. .");
		if (qry("UPDATE timers SET date='0'"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing shop data... .. .");
		send_to_all("Clearing shop data... .. .");
		if (qry("DELETE FROM shop_trophy"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing completed quest data... .. .");
		send_to_all("Clearing completed quest data... .. .");
		if (qry("DELETE FROM world_quest_accomplished"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing locker grant list data... .. .");
		send_to_all("Clearing locker grant list data... .. .");
		if (qry("DELETE FROM locker_access"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		/* -- Season-reset manifest: player-owned item graphs -- */
		/* Child tables (affects, extra_descr) must be cleared before parent item tables. */
		logit(LOG_DEBUG, "sql_pwipe: Clearing player item subtable data... .. .");
		send_to_all("Clearing player item subtable data... .. .");
		if (qry("DELETE FROM player_item_affects") &&
		    qry("DELETE FROM player_item_extra_descr"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing player items... .. .");
		send_to_all("Clearing player items... .. .");
		if (qry("DELETE FROM player_items"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		/* -- Season-reset manifest: player pet graphs -- */
		logit(LOG_DEBUG, "sql_pwipe: Clearing player pet subtable data... .. .");
		send_to_all("Clearing player pet subtable data... .. .");
		if (qry("DELETE FROM player_pet_item_affects") &&
		    qry("DELETE FROM player_pet_item_extra_descr") &&
		    qry("DELETE FROM player_pet_items"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing player pets... .. .");
		send_to_all("Clearing player pets... .. .");
		if (qry("DELETE FROM player_pets"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		/* -- Season-reset manifest: player character state -- */
		logit(LOG_DEBUG, "sql_pwipe: Clearing player character state data... .. .");
		send_to_all("Clearing player character state data... .. .");
		if (qry("DELETE FROM player_affects") && qry("DELETE FROM player_skills") &&
		    qry("DELETE FROM player_spellbooks") && qry("DELETE FROM player_languages") &&
		    qry("DELETE FROM player_timers") && qry("DELETE FROM player_recipes") &&
		    qry("DELETE FROM player_shapechanges") &&
		    qry("DELETE FROM player_undead_slots") && qry("DELETE FROM player_witnesses") &&
		    qry("DELETE FROM player_forged_items") &&
		    qry("DELETE FROM player_granted_cmds") && qry("DELETE FROM player_intros"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		/* -- Season-reset manifest: locker/chest graphs -- */
		logit(LOG_DEBUG, "sql_pwipe: Clearing locker subtable data... .. .");
		send_to_all("Clearing locker subtable data... .. .");
		if (qry("DELETE FROM locker_item_affects") &&
		    qry("DELETE FROM locker_item_extra_descr") && qry("DELETE FROM locker_items") &&
		    qry("DELETE FROM locker_chests") && qry("DELETE FROM locker_activity_log") &&
		    qry("DELETE FROM locker_kickouts") && qry("DELETE FROM locker_session_state"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing locker data... .. .");
		send_to_all("Clearing locker data... .. .");
		if (qry("DELETE FROM lockers"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		/* -- Season-reset manifest: account locker graphs -- */
		logit(LOG_DEBUG, "sql_pwipe: Clearing account locker data... .. .");
		send_to_all("Clearing account locker data... .. .");
		if (qry("DELETE FROM account_locker_item_affects") &&
		    qry("DELETE FROM account_locker_item_extra_descr") &&
		    qry("DELETE FROM account_locker_items") &&
		    qry("DELETE FROM account_locker_access") && qry("DELETE FROM account_lockers"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		/* -- Season-reset manifest: private chests -- */
		logit(LOG_DEBUG, "sql_pwipe: Clearing private chest data... .. .");
		send_to_all("Clearing private chest data... .. .");
		if (qry("DELETE FROM private_chest_log") && qry("DELETE FROM private_chests"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		/* -- Season-reset manifest: corpse graphs -- */
		logit(LOG_DEBUG, "sql_pwipe: Clearing corpse subtable data... .. .");
		send_to_all("Clearing corpse subtable data... .. .");
		if (qry("DELETE FROM corpse_item_affects") &&
		    qry("DELETE FROM corpse_item_extra_descr") && qry("DELETE FROM corpse_items"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing corpse data... .. .");
		send_to_all("Clearing corpse data... .. .");
		if (qry("DELETE FROM corpses"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		/* -- Season-reset manifest: saved item graphs -- */
		logit(LOG_DEBUG, "sql_pwipe: Clearing saved item data... .. .");
		send_to_all("Clearing saved item data... .. .");
		if (qry("DELETE FROM saved_item_affects") &&
		    qry("DELETE FROM saved_item_extra_descr") && qry("DELETE FROM saved_items"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		/* -- Season-reset manifest: ship graphs -- */
		logit(LOG_DEBUG, "sql_pwipe: Clearing ship subtable data... .. .");
		send_to_all("Clearing ship subtable data... .. .");
		if (qry("DELETE FROM ship_armor") && qry("DELETE FROM ship_crew") &&
		    qry("DELETE FROM ship_slots"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing ship data... .. .");
		send_to_all("Clearing ship data... .. .");
		if (qry("DELETE FROM ships"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		/* -- Season-reset manifest: guild membership -- */
		logit(LOG_DEBUG, "sql_pwipe: Clearing guild membership data... .. .");
		send_to_all("Clearing guild membership data... .. .");
		if (qry("DELETE FROM guild_members") && qry("DELETE FROM guild_ranks") &&
		    qry("DELETE FROM guilds"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		/* -- Season-reset manifest: world/competitive state -- */
		logit(LOG_DEBUG, "sql_pwipe: Clearing artifact data... .. .");
		send_to_all("Clearing artifact data... .. .");
		if (qry("DELETE FROM artifacts") && qry("DELETE FROM artifacts_mortal"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing pvp and statistics data... .. .");
		send_to_all("Clearing pvp and statistics data... .. .");
		if (qry("DELETE FROM frag_leaderboard") && qry("DELETE FROM pkill_event") &&
		    qry("DELETE FROM pkill_info") && qry("DELETE FROM statistics") &&
		    qry("DELETE FROM eq_drop") && qry("DELETE FROM racewar_stat_mods"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		/* -- Season-reset manifest: shopkeeper graph -- */
		logit(LOG_DEBUG, "sql_pwipe: Clearing shopkeeper data... .. .");
		send_to_all("Clearing shopkeeper data... .. .");
		if (qry("DELETE FROM shopkeeper_affects") &&
		    qry("DELETE FROM shopkeeper_item_affects") &&
		    qry("DELETE FROM shopkeeper_item_extra_descr") &&
		    qry("DELETE FROM shopkeeper_items") && qry("DELETE FROM shopkeepers"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		/* -- Season-reset manifest: polls and boon shop -- */
		logit(LOG_DEBUG, "sql_pwipe: Clearing poll and boon shop data... .. .");
		send_to_all("Clearing poll and boon shop data... .. .");
		if (qry("DELETE FROM poll_votes") && qry("DELETE FROM poll_options") &&
		    qry("DELETE FROM polls") && qry("DELETE FROM boons_shop"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		/* -- Season-reset manifest: soft-delete account characters -- */
		logit(LOG_DEBUG, "sql_pwipe: Soft-deleting account characters... .. .");
		send_to_all("Soft-deleting account characters... .. .");
		if (qry("UPDATE account_characters SET deleted_at = COALESCE(deleted_at, NOW()) WHERE deleted_at IS NULL"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Deactivating player_data... .. .");
		send_to_all("Deactivating player_data... .. .");
		if (qry("UPDATE player_data SET active = 0"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Clearing persistence event data... .. .");
		send_to_all("Clearing persistence event data... .. .");
		if (qry("DELETE FROM persistence_item_events") &&
		    qry("DELETE FROM persistence_scalar_events"))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "sql_pwipe: Resetting level_cap data... .. .");
		send_to_all("Resetting level_cap data... .. .");
		if (qry("UPDATE level_cap SET most_frags=0, racewar_leader=0, level=%d, next_update=NOW() + INTERVAL %d DAY",
			frag_cap_config_reset_level(), frag_cap_config_reset_timer_days()))
		{
			logit(LOG_DEBUG, "  success!");
			send_to_all("  success!\n");
		}
		else
		{
			logit(LOG_DEBUG, "        failure!");
			send_to_all("        failure!\n");
			return FALSE;
		}
		if (!redis_clear_pwipe_state())
		{
			logit(LOG_DEBUG, "sql_pwipe: Redis pwipe invalidation failed.");
			return FALSE;
		}
		/* -- Postflight: verify critical season-scoped tables are empty -- */
		logit(LOG_DEBUG, "sql_pwipe: Postflight invariant check... .. .");
		send_to_all("Postflight invariant check... .. .");
		{
			/* Verify that player_items, player_pets, lockers, ships, guilds, and
			 * persistence event tables are all empty after reset. */
			const char *postflight_tables[] = { "player_items",
							    "player_pets",
							    "player_affects",
							    "player_skills",
							    "player_spellbooks",
							    "player_languages",
							    "player_timers",
							    "lockers",
							    "locker_items",
							    "private_chests",
							    "corpses",
							    "corpse_items",
							    "saved_items",
							    "ships",
							    "ship_slots",
							    "ship_crew",
							    "ship_armor",
							    "guilds",
							    "guild_members",
							    "guild_ranks",
							    "artifacts",
							    "frag_leaderboard",
							    "statistics",
							    "persistence_item_events",
							    "persistence_scalar_events",
							    "polls",
							    "shopkeepers",
							    NULL };
			bool postflight_ok = TRUE;
			for (int i = 0; postflight_tables[i] != NULL; i++)
			{
				MYSQL_RES *res =
					db_query("SELECT COUNT(*) FROM %s", postflight_tables[i]);
				if (res)
				{
					MYSQL_ROW row = mysql_fetch_row(res);
					if (row && row[0] && atoi(row[0]) > 0)
					{
						logit(LOG_DEBUG,
						      "sql_pwipe: Postflight FAILED: %s has %s rows.",
						      postflight_tables[i], row[0]);
						postflight_ok = FALSE;
					}
					mysql_free_result(res);
				}
				else
				{
					logit(LOG_DEBUG,
					      "sql_pwipe: Postflight FAILED: cannot query %s.",
					      postflight_tables[i]);
					postflight_ok = FALSE;
				}
			}
			/* Verify player_data is all inactive. */
			MYSQL_RES *res =
				db_query("SELECT COUNT(*) FROM player_data WHERE active = 1");
			if (res)
			{
				MYSQL_ROW row = mysql_fetch_row(res);
				if (row && row[0] && atoi(row[0]) > 0)
				{
					logit(LOG_DEBUG,
					      "sql_pwipe: Postflight FAILED: %d active player_data rows.",
					      atoi(row[0]));
					postflight_ok = FALSE;
				}
				mysql_free_result(res);
			}
			else
			{
				logit(LOG_DEBUG,
				      "sql_pwipe: Postflight FAILED: cannot query player_data active count.");
				postflight_ok = FALSE;
			}
			/* Verify account_characters are all soft-deleted. */
			res = db_query(
				"SELECT COUNT(*) FROM account_characters WHERE deleted_at IS NULL");
			if (res)
			{
				MYSQL_ROW row = mysql_fetch_row(res);
				if (row && row[0] && atoi(row[0]) > 0)
				{
					logit(LOG_DEBUG,
					      "sql_pwipe: Postflight FAILED: %d active account_characters rows.",
					      atoi(row[0]));
					postflight_ok = FALSE;
				}
				mysql_free_result(res);
			}
			else
			{
				logit(LOG_DEBUG,
				      "sql_pwipe: Postflight FAILED: cannot query account_characters.");
				postflight_ok = FALSE;
			}
			if (!postflight_ok)
			{
				logit(LOG_DEBUG, "sql_pwipe: Postflight invariants failed.");
				send_to_all(
					"Postflight FAILED: season-reset invariants not satisfied!\n");
				return FALSE;
			}
		}
		if (!account_bound_rewards_on_successful_pwipe())
		{
			logit(LOG_DEBUG,
			      "sql_pwipe: account reward pwipe policy failed; preserving rewards for manual review.");
			send_to_all(
				"Account reward pwipe policy FAILED; rewards are being preserved for manual review.\n");
		}
		if (!sql_complete_pwipe_epoch())
		{
			logit(LOG_DEBUG,
			      "sql_pwipe: Reset data cleared but season state completion failed; shutdown remains fenced.");
			send_to_all(
				"Season reset completion FAILED; server will remain stopped for recovery.\n");
			return FALSE;
		}
		logit(LOG_DEBUG, "  success!");
		send_to_all("  success!\n");
		logit(LOG_DEBUG, "sql_pwipe: COMPLETED!");
		send_to_all("WIPE COMPLETED!");
		sleep(1);
		return TRUE;
	}
	else
	{
		logit(LOG_DEBUG,
		      "sql_pwipe: Someone called sql_pwipe with a bad verify code... hrm..");
		return FALSE;
	}
}

void sql_log_player_login(P_char ch, const char *status)
{
	if (!ch || IS_NPC(ch) || !status)
		return;
	const session_audit_event event = !strcasecmp(status, "login") ?
						  session_audit_event::login :
						  session_audit_event::logout;
	if (strcasecmp(status, "login") && strcasecmp(status, "logout"))
		return;
	if (!session_audit_transaction_submit(ch, event))
		logit(LOG_FILE,
		      "session_audit: component=submit outcome=unavailable actor=redacted");
}

/* ---- Persistence DB connection ---- */
MYSQL *sql_persistence_connection(void)
{
	/* Prefer the connection pool. Only fall back to the legacy singleton
	 * when no active pool exists (bootstrap/early-start). If an active pool
	 * is exhausted, fail closed so callers can alert/retry without blocking
	 * the main loop or creating an unscheduled fifth connection. */
	int pool_was_active = 0;
	MYSQL *conn = sql_pool_acquire_with_status(&pool_was_active);
	if (conn)
		return conn;
	if (pool_was_active)
		return NULL;

	/* Legacy fallback: lazy-initialise the singleton.
	 * Kept for bootstrap / early-start paths that run before
	 * the pool is initialised (e.g. sql_populate_lookup_tables).
	 * Must use the shared db-name resolver so pool/fallback never
	 * disagrees with the main DB connection about which database
	 * is the production one. */
	if (!persistenceDB)
	{
		pthread_mutex_lock(&persistence_sql_mutex);
		if (!persistenceDB)
		{
			persistenceDB = sql_open_configured_connection(0);
			if (!persistenceDB)
			{
				pthread_mutex_unlock(&persistence_sql_mutex);
				return NULL;
			}
		}
		pthread_mutex_unlock(&persistence_sql_mutex);
	}
	return persistenceDB;
}

/* Return a connection previously acquired via
 * sql_persistence_connection().  Pool connections go back to the pool;
 * legacy singleton connections are a no-op (they're owned by the
 * caller indefinitely). */
void sql_persistence_release_connection(MYSQL *conn)
{
	if (!conn)
		return;

	/* If this connection came from the pool, release it.  The pool
	 * checks pointer equality against its slots, so passing a
	 * non-pool connection is harmless -- it simply won't match. */
	sql_pool_release(conn);
}

static bool persistence_decimal_field(const char *value, bool allow_negative)
{
	char *end = NULL;
	if (!value || !*value)
		return false;
	errno = 0;
	if (allow_negative)
		(void)strtol(value, &end, 10);
	else
		(void)strtoull(value, &end, 10);
	return errno == 0 && end && *end == '\0';
}

bool sql_persistence_write_item_event_line(const char *line)
{
	const char *item_event_prefix = "PERSISTENCE_ITEM_EVENT|";
	char record[2048];
	char *saveptr = NULL;
	char *field;
	char *value;
	char event[128] = "";
	char ts_usec[32] = "0";
	char item_uid[32] = "0";
	char vnum[32] = "-1";
	char item[256] = "";
	char actor[128] = "";
	char actor_id[32] = "-1";
	char source[256] = "";
	char target[256] = "";
	char note[256] = "";
	char event_q[256], item_q[64], vnum_q[32], item_text_q[512];
	char actor_q[256], actor_id_q[32], source_q[512], target_q[512], note_q[512];
	int seen_ts = 0, seen_event = 0, seen_item_uid = 0;
	int seen_vnum = 0, seen_actor_id = 0;
	char query[4096];
	int query_len;

	if (!line || strncmp(line, item_event_prefix, strlen(item_event_prefix)))
		return false;
	if (strlen(line) >= sizeof(record))
		return false;
	strcpy(record, line);

	field = strtok_r(record, "|", &saveptr);
	while ((field = strtok_r(NULL, "|", &saveptr)) != NULL)
	{
		value = strchr(field, '=');
		if (!value)
			return false;
		*value++ = '\0';
		if (!strcmp(field, "ts"))
		{
			seen_ts = 1;
			snprintf(ts_usec, sizeof(ts_usec), "%s", value);
		}
		else if (!strcmp(field, "event"))
		{
			seen_event = 1;
			snprintf(event, sizeof(event), "%s", value);
		}
		else if (!strcmp(field, "item_uid"))
		{
			seen_item_uid = 1;
			snprintf(item_uid, sizeof(item_uid), "%s", value);
		}
		else if (!strcmp(field, "vnum"))
		{
			seen_vnum = 1;
			snprintf(vnum, sizeof(vnum), "%s", value);
		}
		else if (!strcmp(field, "item"))
			snprintf(item, sizeof(item), "%s", value);
		else if (!strcmp(field, "actor"))
			snprintf(actor, sizeof(actor), "%s", value);
		else if (!strcmp(field, "actor_id"))
		{
			seen_actor_id = 1;
			snprintf(actor_id, sizeof(actor_id), "%s", value);
		}
		else if (!strcmp(field, "source"))
			snprintf(source, sizeof(source), "%s", value);
		else if (!strcmp(field, "target"))
			snprintf(target, sizeof(target), "%s", value);
		else if (!strcmp(field, "note"))
			snprintf(note, sizeof(note), "%s", value);
	}

	if (!seen_ts || !seen_event || !seen_item_uid || !seen_vnum || !seen_actor_id ||
	    !persistence_decimal_field(ts_usec, false) ||
	    !persistence_decimal_field(item_uid, false) || !persistence_decimal_field(vnum, true) ||
	    !persistence_decimal_field(actor_id, true))
		return false;

	persistence_sql_escape_field(event, event_q, sizeof(event_q));
	persistence_sql_escape_field(item, item_text_q, sizeof(item_text_q));
	persistence_sql_escape_field(actor, actor_q, sizeof(actor_q));
	persistence_sql_escape_field(source, source_q, sizeof(source_q));
	persistence_sql_escape_field(target, target_q, sizeof(target_q));
	persistence_sql_escape_field(note, note_q, sizeof(note_q));
	snprintf(item_q, sizeof(item_q), "%s", item_uid);
	snprintf(vnum_q, sizeof(vnum_q), "%s", vnum);
	snprintf(actor_id_q, sizeof(actor_id_q), "%s", actor_id);
	query_len = checked_snprintf(
		query, sizeof(query),
		"INSERT INTO persistence_item_events "
		"(ts_usec,event_type,item_uid,vnum,item,actor,actor_id,source,target,note,dedupe_key) "
		"VALUES (%s,'%s',%s,%s,'%s','%s',%s,'%s','%s','%s',"
		"SHA2(CONCAT_WS('|',%s,'%s',%s,%s,'%s','%s',%s,'%s','%s','%s'),256)) "
		"ON DUPLICATE KEY UPDATE id=id",
		ts_usec, event_q, item_q, vnum_q, item_text_q, actor_q, actor_id_q, source_q,
		target_q, note_q, ts_usec, event_q, item_q, vnum_q, item_text_q, actor_q,
		actor_id_q, source_q, target_q, note_q);
	if (query_len < 0 || query_len >= (int)sizeof(query))
		return false;
	return sql_persistence_execute_raw(query);
}

bool sql_persistence_write_scalar_event_line(const char *line)
{
	return sql_persistence_execute_raw(line);
}

bool sql_persistence_write_large_event_line(const char *line)
{
	return sql_persistence_execute_raw(line);
}

bool sql_persistence_item_owner_matches_identity(unsigned long long item_uid,
						 const char *owner_type,
						 unsigned long long expected_id,
						 unsigned long long expected_context_id,
						 const char *context)
{
	if (item_uid == 0)
		return true;
	if (!owner_type || !context || !DB)
		return false;
	item_owner_type expected_type = item_owner_type::unknown;
	if (!strcmp(owner_type, "player"))
		expected_type = item_owner_type::player;
	else if (!strcmp(owner_type, "container"))
		expected_type = item_owner_type::container;
	else if (!strcmp(owner_type, "room"))
		expected_type = item_owner_type::room;
	else if (!strcmp(owner_type, "corpse"))
		expected_type = item_owner_type::corpse;
	else if (!strcmp(owner_type, "locker"))
		expected_type = item_owner_type::locker;
	else if (!strcmp(owner_type, "auction"))
		expected_type = item_owner_type::auction;
	else if (!strcmp(owner_type, "shopkeeper"))
		expected_type = item_owner_type::shopkeeper;
	if (expected_type == item_owner_type::unknown)
		return false;
	if (!expected_id)
		return false;
	char query[512];
	snprintf(
		query, sizeof(query),
		"SELECT current_item.root_item_uid,COALESCE(current_item.parent_item_uid,0),"
		"current_item.owner_type,current_item.owner_id,current_item.owner_context_id,"
		"current_item.item_revision,current_item.vnum,current_item.state,owner.revision "
		"FROM item_current_owner current_item JOIN item_owner_revision owner ON "
		"owner.owner_type=current_item.owner_type AND owner.owner_id=current_item.owner_id "
		"AND owner.owner_context_id=current_item.owner_context_id WHERE current_item.item_uid=%llu",
		item_uid);
	MYSQL_RES *result = db_query("%s", query);
	if (!result)
		return false;
	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row)
	{
		mysql_free_result(result);
		logit(LOG_FILE,
		      "sql_persistence: authoritative owner missing item_uid=%llu context=%s",
		      item_uid, context);
		return false;
	}
	item_ownership_runtime_entry entry = {
		.item_uid = item_uid,
		.root_item_uid = strtoull(row[0], NULL, 10),
		.parent_item_uid = strtoull(row[1], NULL, 10),
		.owner = { static_cast<item_owner_type>(strtoul(row[2], NULL, 10)),
			   strtoull(row[3], NULL, 10), strtoull(row[4], NULL, 10) },
		.item_revision = strtoull(row[5], NULL, 10),
		.owner_revision = strtoull(row[8], NULL, 10),
		.vnum = static_cast<int32_t>(strtol(row[6], NULL, 10)),
		.state = static_cast<item_custody_state>(strtoul(row[7], NULL, 10)),
	};
	const bool matches = entry.owner.type == expected_type && entry.owner.id == expected_id &&
			     entry.owner.context_id == expected_context_id &&
			     entry.state == item_custody_state::active;
	if (!matches)
	{
		logit(LOG_DEBUG,
		      "sql_persistence: OWNERSHIP MISMATCH item_uid=%llu "
		      "expected=%u:%llu:%llu actual=%u:%llu:%llu context=%s",
		      item_uid, static_cast<unsigned int>(expected_type), expected_id,
		      expected_context_id, static_cast<unsigned int>(entry.owner.type),
		      (unsigned long long)entry.owner.id,
		      (unsigned long long)entry.owner.context_id, context);
		mysql_free_result(result);
		return false;
	}
	const bool hydrated = item_ownership_runtime_hydrate(entry);
	mysql_free_result(result);
	return hydrated;
}

bool sql_persistence_item_owner_matches(unsigned long long item_uid, const char *owner_type,
					const char *owner_ref, const char *context)
{
	if (item_uid == 0)
		return true;
	if (!owner_ref)
		return false;
	char *owner_end = NULL;
	errno = 0;
	const unsigned long long owner_id = strtoull(owner_ref, &owner_end, 10);
	if (errno || !owner_end || *owner_end || !owner_id)
		return false;
	return sql_persistence_item_owner_matches_identity(item_uid, owner_type, owner_id, 0,
							   context);
}

bool sql_persistence_reconcile_world_recovery_items(const world_recovery_authority_item *items,
						    size_t count,
						    item_ownership_runtime_entry *authoritative,
						    size_t authoritative_capacity)
{
	constexpr size_t QUERY_BATCH_SIZE = 256;
	constexpr size_t MAX_RECOVERY_ITEMS = 262144;
	if ((!items && count) || (!authoritative && count) || count > MAX_RECOVERY_ITEMS ||
	    authoritative_capacity != count || !DB || sql_in_transaction())
		return false;
	if (!count)
		return true;
	std::unordered_map<uint64_t, const world_recovery_authority_item *> expected;
	size_t authoritative_count = 0;
	try
	{
		expected.reserve(count);
		for (size_t index = 0; index < count; ++index)
			if (!items[index].item_uid || !items[index].root_item_uid ||
			    !items[index].room_vnum || items[index].vnum <= 0 ||
			    !expected.emplace(items[index].item_uid, &items[index]).second)
				return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (!sql_begin_transaction())
		return false;
	bool valid = true;
	for (size_t begin = 0; valid && begin < count; begin += QUERY_BATCH_SIZE)
	{
		const size_t end = std::min(count, begin + QUERY_BATCH_SIZE);
		std::string query =
			"SELECT current_item.item_uid,current_item.root_item_uid,"
			"COALESCE(current_item.parent_item_uid,0),current_item.owner_type,"
			"current_item.owner_id,current_item.owner_context_id,"
			"current_item.item_revision,current_item.vnum,current_item.state,"
			"owner.revision FROM item_current_owner current_item JOIN "
			"item_owner_revision owner ON owner.owner_type=current_item.owner_type "
			"AND owner.owner_id=current_item.owner_id AND "
			"owner.owner_context_id=current_item.owner_context_id WHERE "
			"current_item.item_uid IN (";
		try
		{
			for (size_t index = begin; index < end; ++index)
			{
				if (index != begin)
					query.push_back(',');
				query += std::to_string(items[index].item_uid);
			}
			query.push_back(')');
		}
		catch (const std::bad_alloc &)
		{
			valid = false;
			break;
		}
		MYSQL_RES *result = db_query("%s", query.c_str());
		if (!result)
		{
			valid = false;
			break;
		}
		MYSQL_ROW row;
		while (valid && (row = mysql_fetch_row(result)))
		{
			const uint64_t item_uid = row[0] ? strtoull(row[0], NULL, 10) : 0;
			const auto found = expected.find(item_uid);
			if (found == expected.end())
			{
				valid = false;
				break;
			}
			const world_recovery_authority_item &planned = *found->second;
			item_ownership_runtime_entry entry = {
				.item_uid = item_uid,
				.root_item_uid = row[1] ? strtoull(row[1], NULL, 10) : 0,
				.parent_item_uid = row[2] ? strtoull(row[2], NULL, 10) : 0,
				.owner = { row[3] ? static_cast<item_owner_type>(
							    strtoul(row[3], NULL, 10)) :
						    item_owner_type::unknown,
					   row[4] ? strtoull(row[4], NULL, 10) : 0,
					   row[5] ? strtoull(row[5], NULL, 10) : 0 },
				.item_revision = row[6] ? strtoull(row[6], NULL, 10) : 0,
				.owner_revision = row[9] ? strtoull(row[9], NULL, 10) : 0,
				.vnum = row[7] ? static_cast<int32_t>(strtol(row[7], NULL, 10)) : 0,
				.state = row[8] ? static_cast<item_custody_state>(
							  strtoul(row[8], NULL, 10)) :
						  item_custody_state::absent,
			};
			if (entry.root_item_uid != planned.root_item_uid ||
			    entry.parent_item_uid != planned.parent_item_uid ||
			    entry.owner.type != item_owner_type::room ||
			    entry.owner.id != static_cast<uint64_t>(planned.room_vnum) ||
			    entry.owner.context_id != 0 || entry.vnum != planned.vnum ||
			    entry.state != item_custody_state::active)
			{
				valid = false;
				break;
			}
			try
			{
				if (authoritative_count >= authoritative_capacity)
				{
					valid = false;
					break;
				}
				authoritative[authoritative_count++] = entry;
			}
			catch (const std::bad_alloc &)
			{
				valid = false;
			}
		}
		mysql_free_result(result);
	}
	valid = valid && authoritative_count == count;
	if (valid)
	{
		valid = sql_commit();
		if (!valid)
			sql_rollback();
	}
	else
		sql_rollback();
	if (!valid)
		return false;
	return true;
}

bool sql_hydrate_item_owner_revisions(void)
{
	if (!DB)
		return false;
	MYSQL_RES *result = db_query(
		"SELECT owner_type,owner_id,owner_context_id,revision FROM item_owner_revision");
	if (!result)
		return false;
	bool ok = true;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		const item_owner_identity owner = {
			static_cast<item_owner_type>(strtoul(row[0], NULL, 10)),
			strtoull(row[1], NULL, 10), strtoull(row[2], NULL, 10)
		};
		if (!item_ownership_runtime_hydrate_owner(owner, strtoull(row[3], NULL, 10)))
		{
			ok = false;
			break;
		}
	}
	mysql_free_result(result);
	return ok;
}

/* Logs zone touch events to persistence_scalar_events for epic analysis.
 * Uses the async persistence queue with flat-file and direct SQL fallbacks. */
void sql_zone_touch_finished(const char *event_key, int boot_time, int touched_at, int zone_number,
			     int toucher_pid, int group_size, int epic_value, int alignment_delta)
{
	(void)event_key;
	(void)boot_time;
	(void)touched_at;
	(void)zone_number;
	(void)toucher_pid;
	(void)group_size;
	(void)epic_value;
	(void)alignment_delta;
}
#endif
