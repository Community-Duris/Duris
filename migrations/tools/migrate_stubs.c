// migrate_stubs.c
// stubs for migrate_pfiles tool

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <mysql.h>
#include "../../src/core/structs.h"
#include "../../src/sql/sql.h"
#include "../../src/account/account.h"
#include "../../src/ships/ships.h"
#include "../../src/guild/assocs.h"

using namespace std;

// binary format sizes (64-bit linux)
static int short_size = 2;
static int int_size = 4;
static int long_size = 8;

// binary read helpers for migration
ush_int mig_getShort(char **buf)
{
	ush_int s;
	bcopy(*buf, &s, short_size);
	*buf += short_size;
	return s;
}

uint mig_getInt(char **buf)
{
	uint i;
	bcopy(*buf, &i, int_size);
	*buf += int_size;
	return i;
}

long mig_getLong(char **buf)
{
	long l;
	bcopy(*buf, &l, long_size);
	*buf += long_size;
	return l;
}

char *mig_getString(char **buf)
{
	int len = (int)mig_getShort(buf);
	if (len == 0)
		return NULL;
	char *s = (char *)malloc(len + 1);
	strncpy(s, *buf, len);
	s[len] = 0;
	*buf += len;
	return s;
}

// global vars that some code expects
int mem_use[20];
struct mm_ds *dead_trophy_pool = NULL;
struct mm_ds *dead_mob_pool = NULL;
struct mm_ds *dead_pconly_pool = NULL;
int RUNNING_PORT = 7777;
P_index obj_index = NULL;
struct index_data *mob_index = NULL;
struct room_data *world = NULL;
struct zone_data *zone_table = NULL;
int top_of_world = 0;

// database connection
MYSQL *DB = NULL;

// ship type data - minimal version for migration
const ShipTypeData ship_type_data[MAXSHIPCLASS] = {
	{ 0, "Sloop", 0, 0, 100, 4, 100, 150, 2, 0, 5, 100, 10, 5, 1, 0, 0, 0 },
	{ 1, "Yacht", 0, 0, 120, 5, 120, 175, 3, 0, 6, 110, 10, 5, 5, 0, 0, 0 },
	{ 2, "Clipper", 0, 0, 140, 6, 140, 200, 4, 1, 7, 120, 10, 5, 10, 0, 0, 0 },
	{ 3, "Ketch", 0, 0, 160, 7, 160, 200, 5, 1, 8, 100, 10, 5, 15, 0, 0, 0 },
	{ 4, "Caravel", 0, 0, 180, 8, 180, 225, 6, 2, 9, 90, 10, 5, 20, 0, 0, 0 },
	{ 5, "Carrack", 0, 0, 200, 9, 200, 225, 7, 2, 10, 80, 10, 5, 25, 0, 0, 0 },
	{ 6, "Galleon", 0, 0, 220, 10, 220, 250, 8, 3, 12, 70, 10, 5, 30, 0, 0, 0 },
	{ 7, "Corvette", 0, 0, 240, 11, 240, 250, 9, 3, 14, 85, 10, 5, 35, 0, 0, 0 },
	{ 8, "Destroyer", 0, 0, 260, 12, 260, 250, 10, 4, 16, 95, 10, 5, 40, 0, 0, 0 },
	{ 9, "Frigate", 0, 0, 280, 13, 280, 250, 11, 4, 18, 75, 10, 5, 45, 0, 0, 0 },
	{ 10, "Cruiser", 0, 0, 300, 14, 300, 250, 12, 5, 20, 65, 10, 5, 50, 0, 0, 0 },
	{ 11, "Dreadnought", 0, 0, 350, 16, 350, 250, 14, 6, 25, 55, 10, 5, 55, 0, 0, 0 },
	{ 12, "Zone Ship", 0, 0, 500, 16, 500, 250, 16, 8, 30, 50, 10, 5, 60, 0, 0, 0 },
};

// new_ship - simplified version for migration
P_ship new_ship(int m_class, bool npc)
{
	P_ship ship = (P_ship)malloc(sizeof(struct ShipData));
	if (!ship)
		return NULL;

	memset(ship, 0, sizeof(struct ShipData));
	ship->m_class = m_class;
	ship->mainsail = SHIPTYPE_MAX_SAIL(m_class);

	// init slots
	for (int i = 0; i < MAXSLOTS; i++)
	{
		ship->slot[i].type = 0;
		ship->slot[i].index = 0;
		ship->slot[i].position = 0;
		ship->slot[i].timer = 0;
		ship->slot[i].val0 = 0;
		ship->slot[i].val1 = 0;
		ship->slot[i].val2 = 0;
		ship->slot[i].val3 = 0;
		ship->slot[i].val4 = 0;
	}

	return ship;
}

// ShipSlot::clear
void ShipSlot::clear()
{
	type = 0;
	index = 0;
	position = 0;
	timer = 0;
	val0 = 0;
	val1 = 0;
	val2 = 0;
	val3 = 0;
	val4 = 0;
	desc[0] = '\0';
	status[0] = '\0';
}

// Guild constructor
Guild::Guild()
{
	memset(name, 0, sizeof(name));
	memset(titles, 0, sizeof(titles));
	racewar = 0;
	id_number = 0;
	prestige = 0;
	construction = 0;
	platinum = 0;
	gold = 0;
	silver = 0;
	copper = 0;
	member_count = 0;
	bits = 0;
	overmax = 0;
	frags.frags = 0;
	frags.top_frags = 0;
	frags.topfragger[0] = '\0';
	members = NULL;
	next_guild = NULL;
}

Guild::~Guild()
{
	// members should be freed by caller
}

// other guild methods - stubs
Guild::Guild(char *_name, unsigned int _racewar, unsigned int _id_number, unsigned long _prestige,
	     unsigned long _construction, unsigned long _money, unsigned int _bits)
{
	// stub
}

// Guild::save reports success without touching the database: the tool
// persists guilds through sql_save_guild() below, not through this method.
bool Guild::save()
{
	return true;
}
unsigned int Guild::get_max_members()
{
	return 100;
}
unsigned int Guild::max_size()
{
	return 100;
}
void Guild::add_frags(P_char ch, long new_frags) {}
void Guild::frag_remove(P_char member) {}
void Guild::apply(P_char member, P_char applicant) {}
bool Guild::add_member(P_char ch, int rank)
{
	return false;
}
int Guild::update()
{
	return 0;
}
void Guild::update_member(P_char ch) {}
void Guild::update_bits(P_char ch) {}
void Guild::secede(P_char ch) {}
bool Guild::is_allied_with(P_Guild ally)
{
	return false;
}
bool Guild::is_enemy(P_char enemy)
{
	return false;
}
P_Alliance Guild::get_alliance()
{
	return NULL;
}
void Guild::display(P_char ch) {}
void Guild::ledger(P_char ch, char *args) {}
bool Guild::is_clan()
{
	return false;
}
bool Guild::is_guild()
{
	return true;
}
void Guild::kick(P_char victim) {}
void Guild::kick(P_char kicker, char *char_name) {}
bool Guild::sub_money(int p, int g, int s, int c)
{
	return false;
}
void Guild::challenge(P_char member, P_char victim) {}
void Guild::deposit(P_char member, int p, int g, int s, int c) {}
void Guild::enroll(P_char member, P_char victim) {}
void Guild::fine(P_char member, char *target, int p, int g, int s, int c) {}
void Guild::home(P_char member) {}
void Guild::ostracize(P_char member, char *target) {}
void Guild::punish(P_char member, P_char victim) {}
void Guild::rank(P_char member, P_char victim, char *rank_change) {}
void Guild::title(P_char member, P_char victim, char *new_title) {}
void Guild::withdraw(P_char member, int p, int g, int s, int c) {}
void Guild::name_title(P_char member, char *title_info) {}
void Guild::initialize() {}
void Guild::remove_member_from_list(P_char ch) {}
void Guild::write_transaction_to_ledger(const char *name, const char *trans_type,
					const char *coin_str)
{
}
void Guild::title_trim(char *raw_title, char *good_title) {}
int Guild::max_assoc_size()
{
	return 100;
}
void Guild::default_title(P_char ch) {}
void Guild::update_online_members() {}
void Guild::add_points_from_epics(P_char ch, int epics, int epic_type) {}

// other stubs
void send_to_char(const char *txt, P_char ch) {}
void obj_to_room(void *object, int room) {}
int wear(P_char ch, P_obj obj_object, int keyword, bool showit)
{
	return 0;
}
void writeSavedItem(void *obj) {}
void all_affects(void *ch, int i) {}
P_obj read_object(int number, int type)
{
	return NULL;
}
void obj_to_obj(void *o1, void *o2) {}
void obj_to_char(P_obj obj, P_char ch) {}
void str_free(char *source)
{
	if (source)
		free(source);
}
int GET_LEVEL(P_char ch)
{
	return 0;
}
int vitality_limit(P_char ch)
{
	return 0;
}
int calculate_hitpoints(P_char ch)
{
	return 1;
}
int calculate_mana(P_char ch)
{
	return 1;
}
void extract_obj(void *obj, int i)
{
	free(obj);
}
int flag2idx(int flag)
{
	return 0;
}
void update_skills(P_char ch) {}
struct affected_type *get_spell_from_char(P_char ch, int spell, void *context)
{
	return NULL;
}

// memory management
char *str_dup(const char *source)
{
	if (!source)
		return NULL;
	char *dest = (char *)malloc(strlen(source) + 1);
	if (dest)
		strcpy(dest, source);
	return dest;
}

void __free(void *p, char *file, int line)
{
	if (p)
		free(p);
}

// database functions for migration
int initialize_mysql()
{
	const char *host = get_db_host();
	const char *user = get_db_user();
	const char *passwd = get_db_passwd();
	const char *dbname = get_db_name();
	const char *port_str = getenv("DB_PORT");
	int port = port_str ? atoi(port_str) : 3306;

	DB = mysql_init(NULL);
	if (!DB)
	{
		printf("error: mysql_init failed\n");
		return 0;
	}

	if (!mysql_real_connect(DB, host, user, passwd, dbname, port, NULL, 0))
	{
		printf("error: mysql_real_connect failed: %s\n", mysql_error(DB));
		return 0;
	}

	return 1;
}

// reconnect global db on connection loss
static void reconnect_db(void)
{
	if (DB)
	{
		mysql_close(DB);
		DB = NULL;
	}
	initialize_mysql();
}

MYSQL_RES *db_query_at(struct persistence_query_site site, const char *format, ...)
{
	(void)site;
	static char query[65536];
	va_list args;

	va_start(args, format);
	vsnprintf(query, sizeof(query), format, args);
	va_end(args);

	if (mysql_query(DB, query))
	{
		unsigned int err = mysql_errno(DB);
		// CR_SERVER_GONE_ERROR=2006, CR_SERVER_LOST=2013
		if (err == 2006 || err == 2013)
		{
			reconnect_db();
			if (DB && mysql_query(DB, query) == 0)
			{
				return mysql_store_result(DB);
			}
		}
		printf("sql error: %s\nquery: %.200s...\n", mysql_error(DB), query);
		return NULL;
	}

	return mysql_store_result(DB);
}

bool qry_at(struct persistence_query_site site, const char *format, ...)
{
	(void)site;
	static char query[65536];
	va_list args;

	va_start(args, format);
	vsnprintf(query, sizeof(query), format, args);
	va_end(args);

	if (mysql_query(DB, query))
	{
		unsigned int err = mysql_errno(DB);
		// CR_SERVER_GONE_ERROR=2006, CR_SERVER_LOST=2013
		if (err == 2006 || err == 2013)
		{
			reconnect_db();
			if (DB && mysql_query(DB, query) == 0)
			{
				MYSQL_RES *result = mysql_store_result(DB);
				if (result)
					mysql_free_result(result);
				return true;
			}
		}
		printf("sql error: %s\nquery: %.200s...\n", mysql_error(DB), query);
		return false;
	}

	MYSQL_RES *result = mysql_store_result(DB);
	if (result)
	{
		mysql_free_result(result);
	}

	return true;
}

void sql_clear_results()
{
	// drain any remaining results
	while (mysql_next_result(DB) == 0)
	{
		MYSQL_RES *result = mysql_store_result(DB);
		if (result)
			mysql_free_result(result);
	}
}

// more stubs that sql_player.c needs
void ensure_pconly_pool(void) {}

string escape_str(const char *str)
{
	if (!str)
		return string("");
	return string(str);
}

// sql helper functions
char *sql_escape_string(const char *str)
{
	if (!str || !DB)
		return NULL;
	size_t len = strlen(str);
	char *escaped = (char *)malloc(len * 2 + 1);
	if (!escaped)
		return NULL;
	mysql_real_escape_string(DB, escaped, str, len);
	return escaped;
}

static bool sql_run_query(const char *query)
{
	if (!DB || !query)
		return false;
	if (mysql_real_query(DB, query, strlen(query)) != 0)
	{
		unsigned int err = mysql_errno(DB);
		if (err == 2006 || err == 2013)
		{
			reconnect_db();
			if (DB && mysql_real_query(DB, query, strlen(query)) == 0)
			{
				MYSQL_RES *result = mysql_store_result(DB);
				if (result)
					mysql_free_result(result);
				return true;
			}
		}
		printf("sql error: %s\nquery: %.200s...\n", mysql_error(DB), query);
		return false;
	}
	MYSQL_RES *result = mysql_store_result(DB);
	if (result)
		mysql_free_result(result);
	return true;
}

// ship save functions
static bool sql_save_ship_armor(int ship_id, P_ship ship)
{
	if (!DB || !ship || ship_id <= 0)
		return false;
	for (int i = 0; i < 4; i++)
	{
		char query[256];
		snprintf(query, sizeof(query),
			 "insert into ship_armor (ship_id, side, armor, internal) "
			 "values (%d, %d, %d, %d) "
			 "on duplicate key update armor=%d, internal=%d",
			 ship_id, i, ship->armor[i], ship->internal[i], ship->armor[i],
			 ship->internal[i]);
		if (!sql_run_query(query))
			return false;
	}
	return true;
}

static bool sql_save_ship_crew(int ship_id, P_ship ship)
{
	if (!DB || !ship || ship_id <= 0)
		return false;
	char query[512];
	snprintf(
		query, sizeof(query),
		"insert into ship_crew (ship_id, crew_index, sail_skill, guns_skill, rpar_skill, "
		"sail_chief, guns_chief, rpar_chief) "
		"values (%d, %d, %d, %d, %d, %d, %d, %d) "
		"on duplicate key update crew_index=%d, sail_skill=%d, guns_skill=%d, rpar_skill=%d, "
		"sail_chief=%d, guns_chief=%d, rpar_chief=%d",
		ship_id, ship->crew.index, (int)(ship->crew.sail_skill * 1000),
		(int)(ship->crew.guns_skill * 1000), (int)(ship->crew.rpar_skill * 1000),
		ship->crew.sail_chief, ship->crew.guns_chief, ship->crew.rpar_chief,
		ship->crew.index, (int)(ship->crew.sail_skill * 1000),
		(int)(ship->crew.guns_skill * 1000), (int)(ship->crew.rpar_skill * 1000),
		ship->crew.sail_chief, ship->crew.guns_chief, ship->crew.rpar_chief);
	return sql_run_query(query);
}

static bool sql_save_ship_slots(int ship_id, P_ship ship)
{
	if (!DB || !ship || ship_id <= 0)
		return false;
	for (int i = 0; i < MAXSLOTS; i++)
	{
		char query[512];
		snprintf(
			query, sizeof(query),
			"insert into ship_slots (ship_id, slot_index, slot_type, item_index, position, "
			"timer, val0, val1, val2, val3, val4) "
			"values (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d) "
			"on duplicate key update slot_type=%d, item_index=%d, position=%d, "
			"timer=%d, val0=%d, val1=%d, val2=%d, val3=%d, val4=%d",
			ship_id, i, ship->slot[i].type, ship->slot[i].index, ship->slot[i].position,
			ship->slot[i].timer, ship->slot[i].val0, ship->slot[i].val1,
			ship->slot[i].val2, ship->slot[i].val3, ship->slot[i].val4,
			ship->slot[i].type, ship->slot[i].index, ship->slot[i].position,
			ship->slot[i].timer, ship->slot[i].val0, ship->slot[i].val1,
			ship->slot[i].val2, ship->slot[i].val3, ship->slot[i].val4);
		if (!sql_run_query(query))
			return false;
	}
	return true;
}

bool sql_save_ship(P_ship ship)
{
	if (!DB || !ship || !ship->ownername)
		return false;

	char *esc_owner = sql_escape_string(ship->ownername);
	if (!esc_owner)
		return false;
	char *esc_name = sql_escape_string(ship->name ? ship->name : "");

	char query[512];
	snprintf(
		query, sizeof(query),
		"insert into ships (owner_name, ship_name, ship_class, frags, anchor_room, time_played, mainsail) "
		"values ('%s', '%s', %d, %d, %d, %d, %d) "
		"on duplicate key update ship_name='%s', ship_class=%d, frags=%d, anchor_room=%d, "
		"time_played=%d, mainsail=%d",
		esc_owner, esc_name ? esc_name : "", ship->m_class, ship->frags, ship->anchor,
		ship->time, ship->mainsail, esc_name ? esc_name : "", ship->m_class, ship->frags,
		ship->anchor, ship->time, ship->mainsail);

	bool ok = sql_run_query(query);
	if (!ok)
	{
		free(esc_owner);
		if (esc_name)
			free(esc_name);
		return false;
	}

	// get ship id
	snprintf(query, sizeof(query), "select id from ships where owner_name='%s'", esc_owner);
	MYSQL_RES *result = db_query("%s", query);
	free(esc_owner);
	if (esc_name)
		free(esc_name);

	if (!result)
		return false;

	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row)
	{
		mysql_free_result(result);
		return false;
	}

	int ship_id = atoi(row[0]);
	mysql_free_result(result);

	sql_save_ship_armor(ship_id, ship);
	sql_save_ship_crew(ship_id, ship);
	sql_save_ship_slots(ship_id, ship);

	return true;
}

// account save function
bool sql_save_account(struct acct_entry *acc)
{
	if (!DB || !acc || !acc->acct_name)
		return false;

	char *esc_name = sql_escape_string(acc->acct_name);
	char *esc_email = sql_escape_string(acc->acct_email ? acc->acct_email : "");
	char *esc_pass = sql_escape_string(acc->acct_password ? acc->acct_password : "");
	char *esc_conf = sql_escape_string(acc->acct_confirmation ? acc->acct_confirmation : "");

	if (!esc_name || !esc_email || !esc_pass || !esc_conf)
	{
		if (esc_name)
			free(esc_name);
		if (esc_email)
			free(esc_email);
		if (esc_pass)
			free(esc_pass);
		if (esc_conf)
			free(esc_conf);
		return false;
	}

	char query[2048];
	snprintf(
		query, sizeof(query),
		"insert into accounts (account_name, email, password, confirmation_code, "
		"confirmed, confirmation_sent, blocked, last_login, last_good_char, last_evil_char, "
		"flags1, flags2, flags3, flags4) values ('%s', '%s', '%s', '%s', %d, %d, %d, FROM_UNIXTIME(NULLIF(%ld,0)), FROM_UNIXTIME(NULLIF(%ld,0)), FROM_UNIXTIME(NULLIF(%ld,0)), %lu, %lu, %lu, %lu)"
		"on duplicate key update email='%s', password='%s', confirmation_code='%s', "
		"confirmed=%d, confirmation_sent=%d, blocked=%d, last_login=FROM_UNIXTIME(NULLIF(%ld,0)), last_good_char=FROM_UNIXTIME(NULLIF(%ld,0)), last_evil_char=FROM_UNIXTIME(NULLIF(%ld,0)),"
		"flags1=%lu, flags2=%lu, flags3=%lu, flags4=%lu",
		esc_name, esc_email, esc_pass, esc_conf, acc->acct_confirmed,
		acc->acct_confirmation_sent, acc->acct_blocked, acc->acct_last, acc->acct_good,
		acc->acct_evil, acc->acct_flags1, acc->acct_flags2, acc->acct_flags3,
		acc->acct_flags4, esc_email, esc_pass, esc_conf, acc->acct_confirmed,
		acc->acct_confirmation_sent, acc->acct_blocked, acc->acct_last, acc->acct_good,
		acc->acct_evil, acc->acct_flags1, acc->acct_flags2, acc->acct_flags3,
		acc->acct_flags4);

	if (!sql_run_query(query))
	{
		free(esc_name);
		free(esc_email);
		free(esc_pass);
		free(esc_conf);
		return false;
	}

	// save ips
	char del_query[256];
	snprintf(del_query, sizeof(del_query), "delete from account_ips where account_name='%s'",
		 esc_name);
	sql_run_query(del_query);

	for (struct acct_ip *ip = acc->acct_unique_ips; ip; ip = ip->next)
	{
		char *esc_hostname = sql_escape_string(ip->hostname ? ip->hostname : "");
		char *esc_ip = sql_escape_string(ip->ip_address ? ip->ip_address : "");

		if (esc_hostname && esc_ip)
		{
			char ip_query[512];
			snprintf(
				ip_query, sizeof(ip_query),
				"insert into account_ips (account_name, hostname, ip_address, count) "
				"values ('%s', '%s', '%s', %lu)",
				esc_name, esc_hostname, esc_ip, ip->count);
			sql_run_query(ip_query);
		}

		if (esc_hostname)
			free(esc_hostname);
		if (esc_ip)
			free(esc_ip);
	}

	// save characters
	for (struct acct_chars *ch = acc->acct_character_list; ch; ch = ch->next)
	{
		if (!ch->charname)
			continue;

		char *esc_char = sql_escape_string(ch->charname);
		if (!esc_char)
			continue;

		// look up pid from player_data
		int pid = 0;
		char pid_query[256];
		snprintf(pid_query, sizeof(pid_query),
			 "SELECT pid FROM player_data WHERE LOWER(name) = LOWER('%s')", esc_char);
		MYSQL_RES *pid_res = db_query("%s", pid_query);
		if (pid_res)
		{
			MYSQL_ROW pid_row = mysql_fetch_row(pid_res);
			if (pid_row && pid_row[0])
				pid = atoi(pid_row[0]);
			mysql_free_result(pid_res);
		}

		char char_query[512];
		snprintf(
			char_query, sizeof(char_query),
			"insert into account_characters (account_name, char_name, pid, login_count, last_login, blocked, racewar) "
			"values ('%s', '%s', %d, %lu, FROM_UNIXTIME(NULLIF(%ld,0)), %d, %d)"
			"on duplicate key update pid=%d, login_count=%lu, last_login=FROM_UNIXTIME(NULLIF(%ld,0)), blocked=%d, racewar=%d",
			esc_name, esc_char, pid, ch->count, ch->last, ch->blocked, ch->racewar, pid,
			ch->count, ch->last, ch->blocked, ch->racewar);
		sql_run_query(char_query);
		free(esc_char);
	}

	free(esc_name);
	free(esc_email);
	free(esc_pass);
	free(esc_conf);

	return true;
}

// guild save function
bool sql_save_guild(Guild *guild)
{
	if (!DB || !guild)
		return false;

	unsigned int gid = guild->get_id();
	char *esc_name = sql_escape_string(guild->name);
	char *esc_fragger = sql_escape_string(guild->frags.topfragger);

	char query[1024];
	snprintf(query, sizeof(query),
		 "insert into guilds (id, name, racewar, bits, prestige, construction, "
		 "platinum, gold, silver, copper, frags, top_frags, topfragger) "
		 "values (%u, '%s', %u, %u, %lu, %lu, %u, %u, %u, %u, %ld, %ld, '%s') "
		 "on duplicate key update name='%s', racewar=%u, bits=%u, prestige=%lu, "
		 "construction=%lu, platinum=%u, gold=%u, silver=%u, copper=%u, "
		 "frags=%ld, top_frags=%ld, topfragger='%s'",
		 gid, esc_name ? esc_name : "", guild->racewar, guild->bits, guild->prestige,
		 guild->construction, guild->platinum, guild->gold, guild->silver, guild->copper,
		 guild->frags.frags, guild->frags.top_frags, esc_fragger ? esc_fragger : "",
		 esc_name ? esc_name : "", guild->racewar, guild->bits, guild->prestige,
		 guild->construction, guild->platinum, guild->gold, guild->silver, guild->copper,
		 guild->frags.frags, guild->frags.top_frags, esc_fragger ? esc_fragger : "");

	if (esc_name)
		free(esc_name);
	if (esc_fragger)
		free(esc_fragger);

	if (!sql_run_query(query))
		return false;

	// save ranks
	snprintf(query, sizeof(query), "delete from guild_ranks where guild_id=%u", gid);
	sql_run_query(query);
	for (int i = 0; i < ASC_NUM_RANKS; i++)
	{
		char *esc_title = sql_escape_string(guild->titles[i]);
		if (!esc_title)
			continue;
		snprintf(
			query, sizeof(query),
			"insert into guild_ranks (guild_id, rank_index, title) values (%u, %d, '%s')",
			gid, i, esc_title);
		sql_run_query(query);
		free(esc_title);
	}

	// save members with pid lookup from player_data
	snprintf(query, sizeof(query), "delete from guild_members where guild_id=%u", gid);
	sql_run_query(query);
	for (P_member mem = guild->members; mem; mem = mem->next)
	{
		char *esc_mname = sql_escape_string(mem->name);
		if (!esc_mname)
			continue;
		snprintf(
			query, sizeof(query),
			"insert ignore into guild_members (guild_id, player_name, player_pid, bits, debt) "
			"values (%u, '%s', COALESCE((SELECT pid FROM player_data WHERE LOWER(name) = LOWER('%s')), 0), %u, %u)",
			gid, esc_mname, esc_mname, mem->bits, mem->debt);
		sql_run_query(query);
		free(esc_mname);
	}

	return true;
}
