#ifndef __SQL_H_INCLUDED__
#define __SQL_H_INCLUDED__

#include "structs.h"
#include "item_ownership_runtime.h"
#include "persistence_observability.h"
#include "world_recovery_pipeline.h"
#include <stdlib.h>

/* Database connection fields are explicit. Runtime validation rejects missing values. */
static inline const char *get_db_host(void)
{
	const char *val = getenv("DB_HOST");
	return val ? val : "";
}

static inline const char *get_db_user(void)
{
	const char *val = getenv("DB_USER");
	return val ? val : "";
}

static inline const char *get_db_passwd(void)
{
	const char *val = getenv("DB_PASSWD");
	return val ? val : "";
}

static inline const char *get_db_name(void)
{
	const char *val = getenv("DB_NAME");
	return val ? val : "";
}

static inline int get_db_port(void)
{
	const char *val = getenv("DB_PORT");
	return (val && *val) ? atoi(val) : 0;
}

/* legacy macros for backward compatibility - use functions for new code */
#define DB_HOST get_db_host()
#define DB_USER get_db_user()
#define DB_PASSWD get_db_passwd()
#define DB_NAME get_db_name()
#define DB_PORT get_db_port()

#ifdef __NO_MYSQL__
#include "no_mysql/mysql.h"
#else
#include <mysql.h>
#endif
extern MYSQL *DB;
MYSQL *sql_open_configured_connection(unsigned long client_flags);
MYSQL_RES *db_query_at(struct persistence_query_site site, const char *format, ...);
MYSQL_RES *db_query_nolog_at(struct persistence_query_site site, const char *format, ...);
bool sql_observed_execute_at(MYSQL *conn, struct persistence_query_site site,
			     enum persistence_query_context context, const char *sql, size_t len,
			     uint64_t *operation_id);

int load_env_file(void);
int initialize_mysql();
bool sql_populate_lookup_tables();
int sql_save_player_core(P_char ch);
bool sql_load_player_items(P_char ch);
int sql_level_cap(int racewar_side);
// void sql_save_progress( int pid, int delta, const char *type );
void sql_modify_frags(P_char ch, int gain);
void sql_check_level_cap_periodic(void);
void sql_save_pkill(P_char ch, P_char victim);
void sql_webinfo_toggle(P_char ch);
void sql_update_level(P_char ch);
void sql_update_money(P_char ch);
void sql_update_playtime(P_char ch);
void sql_update_epics(P_char ch);
void get_log(P_char ch, char *temp);
void manual_log(P_char ch);
void sql_insert_item(P_char ch, P_obj obj, char *desc);
void sql_insert_new_item(P_char ch, P_obj obj);
void perform_wiki_search(P_char ch, const char *buf);
// use to insert a players IP address into the SQL database
void sql_connectIP(P_char ch);
// used to retrieve the last used IP for a player.
const char *sql_select_IP_info(P_char ch, char *buf, size_t bufSize, time_t *lastConnect = NULL,
			       time_t *lastDisconnect = NULL);
int sql_find_racewar_for_ip(char *ip, int *racewar_side);
// to log disconnect times...
void sql_disconnectIP(P_char ch);
bool qry_at(struct persistence_query_site site, const char *format, ...);
bool sql_persistence_write_item_event_line(const char *line);
bool sql_persistence_write_scalar_event_line(const char *line);
bool sql_persistence_write_large_event_line(const char *line);
bool sql_trace_exec_at(struct persistence_query_site source_site, const char *label,
		       const char *sql, size_t len, bool drain_before, bool drain_after);
void sql_trace_panic(void);

/* Resolve which database name to connect to based on the current
 * running port.  On non-default ports (e.g. dev builds) the live
 * "duris" database is replaced by the "duris_dev" sandbox so the
 * boot-time hack can never accidentally clobber production data.
 * This single helper is used by initialize_mysql(), the legacy
 * persistenceDB fallback, and every slot in sql_pool_init() so the
 * sync DB connection and the async/pool connections always target
 * the same database. */
const char *sql_persistence_db_name(void);

#ifndef __NO_MYSQL__
MYSQL *sql_persistence_connection(void);
void sql_persistence_release_connection(MYSQL *conn);
#endif
bool sql_persistence_execute_raw(const char *sql);
bool sql_persistence_item_owner_matches(unsigned long long item_uid, const char *owner_type,
					const char *owner_ref, const char *context);
bool sql_persistence_item_owner_matches_identity(unsigned long long item_uid,
						 const char *owner_type,
						 unsigned long long owner_id,
						 unsigned long long owner_context_id,
						 const char *context);
bool sql_persistence_reconcile_world_recovery_items(const world_recovery_authority_item *items,
						    size_t count,
						    item_ownership_runtime_entry *authoritative,
						    size_t authoritative_capacity);
bool sql_hydrate_item_owner_revisions(void);
void sql_world_quest_finished(P_char ch, P_obj obj);
int sql_world_quest_done_already(P_char ch, int number);
int sql_world_quest_can_do_another(P_char ch);
void sql_zone_touch_finished(const char *event_key, int boot_time, int touched_at, int zone_number,
			     int toucher_pid, int group_size, int epic_value, int alignment_delta);
void sql_clear_results();
bool sql_run_multi_query(const char *query);

#define db_query(...) db_query_at(PERSISTENCE_QUERY_SITE, __VA_ARGS__)
#define db_query_nolog(...) db_query_nolog_at(PERSISTENCE_QUERY_SITE, __VA_ARGS__)
#define qry(...) qry_at(PERSISTENCE_QUERY_SITE, __VA_ARGS__)
#define sql_trace_exec(label, ...) sql_trace_exec_at(PERSISTENCE_QUERY_SITE, label, __VA_ARGS__)

void send_to_pid_offline(const char *msg, int pid);
void send_offline_messages(P_char ch);

int sql_shop_sell(P_char ch, P_obj obj, int value);
int sql_shop_trophy(P_obj obj);
int sql_quest_finish(P_char ch, P_char giver, int type, int value);
int sql_quest_trophy(P_char giver);

void log_epic_gain(int pid, int type, int type_id, int epics);
void do_sql(P_char ch, char *argument, int cmd);

void update_zone_db();
void update_zone_epic_level(int, int);

void show_frag_trophy(P_char ch, P_char who);

// Frag leaderboard hybrid system - for web statistics
void sql_update_frag_leaderboard(P_char ch);
void sql_update_account_character(P_char ch);
double sql_get_total_donated(const char *account_name);
bool sql_soft_delete_character(long pid);

string get_mud_info(const char *name);
void send_mud_info(const char *name, P_char ch);

string escape_str(const char *str);

#ifndef __NO_MYSQL__
void sql_clear_results_on(MYSQL *conn);
#endif

#include <vector>
using namespace std;

void zone_trophy_update();

#define PLAYERLOG "player"
#define WIZLOG "wiz"
#define QUESTLOG "quest"
#define EXPLOG "exp"
#define CONNECTLOG "connect"

void sql_log(P_char ch, const char *kind, const char *format, ...);
void sql_log_player_login(P_char ch, const char *status);

struct zone_info
{
	int number;
	string name;
	int epic_type;
	float frequency_mod;
	float zone_freq_mod;
	int epic_level;
	bool task_zone;
	bool quest_zone;
	bool trophy_zone;
	int suggested_group_size;
	int epic_payout;
	int difficulty;
};

bool get_zone_info(int zone_number, struct zone_info *info);

bool sql_get_bind_data(int vnum, int *owner_pid, int *timer);
void sql_update_bind_data(int vnum, int *owner_pid, int *timer);

void sql_ship_sunk(char owner);
void sql_get_sincesunk_frags(char owner, float *frags);
void sql_add_sincesunk_frags(char owner, float frags);

bool sql_pwipe(int code_verify);
bool sql_pwipe_crossed_boundary(void);
uint64_t sql_season_epoch(void);
bool sql_verify_pwipe_manifest(void);
bool sql_verify_persistence_schema(void);
bool sql_verify_auction_engines(void);
bool sql_clear_zone_trophy();
/* ---- Persistence layer declarations ---- */
/* (duplicate item/scalar event declarations removed; remaining: */
void log_epic_gain_event(const char *event_key, int pid, int type, int type_id, int epics);

#endif
