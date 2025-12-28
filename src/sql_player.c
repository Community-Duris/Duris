// sql_player.c
// player save/load functions for mysql storage
// part of pfile-to-db migration

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>

#include "structs.h"
#include "utils.h"
#include "prototypes.h"
#include "sql.h"
#include "sql_player.h"
#include "comm.h"
#include "mm.h"
#include "db.h"
#include "account.h"
#include "assocs.h"

// external index tables
extern P_index obj_index;

#ifdef __NO_MYSQL__

// ============================================================================
// stubs when mysql is disabled
// ============================================================================

bool sql_begin_transaction(void) { return false; }
bool sql_commit(void) { return false; }
bool sql_rollback(void) { return false; }
bool sql_in_transaction(void) { return false; }

bool sql_save_player(P_char ch, int type, int room) { return false; }
bool sql_save_player_status(P_char ch, int type, int room) { return false; }
bool sql_save_player_skills(P_char ch) { return false; }
bool sql_save_player_affects(P_char ch) { return false; }
bool sql_save_player_items(P_char ch) { return false; }
bool sql_save_player_witnesses(P_char ch) { return false; }

P_char sql_load_player(const char *name) { return NULL; }
bool sql_player_exists(const char *name) { return false; }
int sql_get_player_pid(const char *name) { return -1; }
bool sql_load_player_status(P_char ch, int pid) { return false; }
bool sql_load_player_skills(P_char ch) { return false; }
bool sql_load_player_affects(P_char ch) { return false; }
bool sql_load_player_items(P_char ch) { return false; }
bool sql_load_player_witnesses(P_char ch) { return false; }

bool sql_delete_player(int pid) { return false; }
bool sql_delete_player_by_name(const char *name) { return false; }

bool sql_save_account(struct acct_entry *acc) { return false; }
struct acct_entry *sql_load_account(const char *name) { return NULL; }
bool sql_account_exists(const char *name) { return false; }
bool sql_link_player_to_account(const char *account_name, int pid) { return false; }

bool sql_save_locker(P_char locker_ch, int owner_pid, int owner_assoc_id) { return false; }
P_char sql_load_locker(int owner_pid, int owner_assoc_id) { return NULL; }
bool sql_locker_exists(int owner_pid, int owner_assoc_id) { return false; }
bool sql_delete_locker(int owner_pid, int owner_assoc_id) { return false; }

bool sql_migrate_player(const char *name) { return false; }
bool sql_verify_player(const char *name) { return false; }
int sql_migrate_all_players(void) { return 0; }

char *sql_escape_string(const char *str) { return NULL; }
void sql_player_error(const char *context, const char *query) { }

#else

// ============================================================================
// globals
// ============================================================================

extern MYSQL *DB;

// track transaction state
static bool in_transaction = false;

// ============================================================================
// transaction helpers
// ============================================================================

bool sql_begin_transaction(void)
{
  if (!DB)
  {
    logit(LOG_DEBUG, "sql_begin_transaction: db not initialized");
    return false;
  }

  if (in_transaction)
  {
    logit(LOG_DEBUG, "sql_begin_transaction: already in transaction");
    return false;
  }

  if (mysql_real_query(DB, "START TRANSACTION", 17) != 0)
  {
    logit(LOG_DEBUG, "sql_begin_transaction: failed: %s", mysql_error(DB));
    return false;
  }

  in_transaction = true;
  return true;
}

bool sql_commit(void)
{
  if (!DB)
  {
    logit(LOG_DEBUG, "sql_commit: db not initialized");
    return false;
  }

  if (!in_transaction)
  {
    logit(LOG_DEBUG, "sql_commit: not in transaction");
    return false;
  }

  if (mysql_real_query(DB, "COMMIT", 6) != 0)
  {
    logit(LOG_DEBUG, "sql_commit: failed: %s", mysql_error(DB));
    in_transaction = false;
    return false;
  }

  in_transaction = false;
  return true;
}

bool sql_rollback(void)
{
  if (!DB)
  {
    logit(LOG_DEBUG, "sql_rollback: db not initialized");
    return false;
  }

  if (!in_transaction)
  {
    logit(LOG_DEBUG, "sql_rollback: not in transaction");
    return false;
  }

  if (mysql_real_query(DB, "ROLLBACK", 8) != 0)
  {
    logit(LOG_DEBUG, "sql_rollback: failed: %s", mysql_error(DB));
    in_transaction = false;
    return false;
  }

  in_transaction = false;
  return true;
}

bool sql_in_transaction(void)
{
  return in_transaction;
}

// ============================================================================
// utility functions
// ============================================================================

// escape string for sql, caller must free
char *sql_escape_string(const char *str)
{
  if (!str || !DB)
    return NULL;

  size_t len = strlen(str);
  // mysql_real_escape_string needs at most len*2+1 bytes
  char *escaped = (char *)malloc(len * 2 + 1);
  if (!escaped)
    return NULL;

  mysql_real_escape_string(DB, escaped, str, len);
  return escaped;
}

// log sql error with context
void sql_player_error(const char *context, const char *query)
{
  if (!DB)
  {
    logit(LOG_DEBUG, "sql_player: %s: db not initialized", context);
    return;
  }

  logit(LOG_DEBUG, "sql_player: %s: %s", context, mysql_error(DB));
  if (query)
  {
    // log first 200 chars of query for debugging
    char truncated[201];
    strncpy(truncated, query, 200);
    truncated[200] = '\0';
    logit(LOG_DEBUG, "sql_player: query: %s...", truncated);
  }
}

// helper to run query and free result
static bool sql_run_query(const char *query)
{
  if (!DB || !query)
    return false;

  if (mysql_real_query(DB, query, strlen(query)) != 0)
  {
    sql_player_error("sql_run_query", query);
    return false;
  }

  // consume any result set
  MYSQL_RES *result = mysql_store_result(DB);
  if (result)
    mysql_free_result(result);

  return true;
}

// ============================================================================
// player existence check
// ============================================================================

bool sql_player_exists(const char *name)
{
  if (!DB || !name)
    return false;

  char *escaped_name = sql_escape_string(name);
  if (!escaped_name)
    return false;

  char query[256];
  snprintf(query, sizeof(query),
           "SELECT 1 FROM player_data WHERE name='%s' LIMIT 1",
           escaped_name);
  free(escaped_name);

  MYSQL_RES *result = db_query("%s", query);
  if (!result)
    return false;

  MYSQL_ROW row = mysql_fetch_row(result);
  bool exists = (row != NULL);
  mysql_free_result(result);

  return exists;
}

int sql_get_player_pid(const char *name)
{
  if (!DB || !name)
    return -1;

  char *escaped_name = sql_escape_string(name);
  if (!escaped_name)
    return -1;

  char query[256];
  snprintf(query, sizeof(query),
           "SELECT pid FROM player_data WHERE name='%s' LIMIT 1",
           escaped_name);
  free(escaped_name);

  MYSQL_RES *result = db_query("%s", query);
  if (!result)
    return -1;

  MYSQL_ROW row = mysql_fetch_row(result);
  int pid = -1;
  if (row && row[0])
    pid = atoi(row[0]);
  mysql_free_result(result);

  return pid;
}

// ============================================================================
// player delete
// ============================================================================

bool sql_delete_player(int pid)
{
  if (!DB || pid <= 0)
    return false;

  char query[128];
  snprintf(query, sizeof(query),
           "DELETE FROM player_data WHERE pid=%d", pid);

  return sql_run_query(query);
}

bool sql_delete_player_by_name(const char *name)
{
  int pid = sql_get_player_pid(name);
  if (pid <= 0)
    return false;
  return sql_delete_player(pid);
}

// ============================================================================
// master save function
// ============================================================================

bool sql_save_player(P_char ch, int type, int room)
{
  if (!ch || !IS_PC(ch))
  {
    logit(LOG_DEBUG, "sql_save_player: invalid char or npc");
    return false;
  }

  if (!DB)
  {
    logit(LOG_DEBUG, "sql_save_player: db not initialized");
    return false;
  }

  // start transaction for atomic save
  if (!sql_begin_transaction())
  {
    logit(LOG_DEBUG, "sql_save_player: failed to start transaction");
    return false;
  }

  // save all components
  if (!sql_save_player_status(ch, type, room))
  {
    logit(LOG_DEBUG, "sql_save_player: failed to save status for %s", GET_NAME(ch));
    sql_rollback();
    return false;
  }

  if (!sql_save_player_skills(ch))
  {
    logit(LOG_DEBUG, "sql_save_player: failed to save skills for %s", GET_NAME(ch));
    sql_rollback();
    return false;
  }

  if (!sql_save_player_affects(ch))
  {
    logit(LOG_DEBUG, "sql_save_player: failed to save affects for %s", GET_NAME(ch));
    sql_rollback();
    return false;
  }

  if (!sql_save_player_items(ch))
  {
    logit(LOG_DEBUG, "sql_save_player: failed to save items for %s", GET_NAME(ch));
    sql_rollback();
    return false;
  }

  if (!sql_save_player_witnesses(ch))
  {
    logit(LOG_DEBUG, "sql_save_player: failed to save witnesses for %s", GET_NAME(ch));
    sql_rollback();
    return false;
  }

  // commit transaction
  if (!sql_commit())
  {
    logit(LOG_DEBUG, "sql_save_player: failed to commit for %s", GET_NAME(ch));
    sql_rollback();
    return false;
  }

  return true;
}

// ============================================================================
// status save (main player data)
// ============================================================================

bool sql_save_player_status(P_char ch, int type, int room)
{
  if (!ch || !IS_PC(ch) || !DB)
    return false;

  int pid = GET_PID(ch);
  bool is_update = (pid > 0 && sql_player_exists(GET_NAME(ch)));

  // build the query
  // this is a big query, we'll use a large buffer
  char query[16384];
  char *q = query;
  int remaining = sizeof(query);
  int written;

  // escape strings that might contain special chars
  char *esc_name = sql_escape_string(GET_NAME(ch) ? GET_NAME(ch) : "");
  char *esc_short = sql_escape_string(ch->player.short_descr ? ch->player.short_descr : "");
  char *esc_long = sql_escape_string(ch->player.long_descr ? ch->player.long_descr : "");
  char *esc_desc = sql_escape_string(ch->player.description ? ch->player.description : "");
  char *esc_title = sql_escape_string(GET_TITLE(ch) ? GET_TITLE(ch) : "");
  char *esc_poofin = sql_escape_string(ch->only.pc->poofIn ? ch->only.pc->poofIn : "");
  char *esc_poofout = sql_escape_string(ch->only.pc->poofOut ? ch->only.pc->poofOut : "");
  char *esc_poofinsnd = sql_escape_string(ch->only.pc->poofInSound ? ch->only.pc->poofInSound : "");
  char *esc_poofoutsnd = sql_escape_string(ch->only.pc->poofOutSound ? ch->only.pc->poofOutSound : "");

  if (is_update)
  {
    written = snprintf(q, remaining,
      "UPDATE player_data SET "
      "short_descr='%s', long_descr='%s', description='%s', title='%s', "
      "m_class=%u, secondary_class=%u, spec=%d, race=%d, racewar=%d, "
      "level=%d, sex=%d, weight=%d, height=%d, size=%d, "
      "hometown=%d, birthplace=%d, orig_birthplace=%d, last_room=%d, "
      "birth_time=%ld, played_time=%d, last_save=%ld, perm_aging=%d, "
      "base_str=%d, base_dex=%d, base_agi=%d, base_con=%d, base_pow=%d, "
      "base_int=%d, base_wis=%d, base_cha=%d, base_kar=%d, base_luk=%d, "
      "mana=%d, base_mana=%d, hit_diff=%d, base_hit=%d, "
      "vitality=%d, base_vitality=%d, spells_memmed_extra=%d, "
      "copper=%d, silver=%d, gold=%d, platinum=%d, "
      "bank_copper=%d, bank_silver=%d, bank_gold=%d, bank_platinum=%d, "
      "exp=%d, epics=%ld, epic_skill_points=%ld, skillpoints=%d, spell_bind_used=%ld, "
      "act=%u, act2=%u, vote=%lu, alignment=%d, "
      "prestige=%d, assoc_id=%d, guild_status=%u, "
      "time_left_guild=%ld, nb_left_guild=%d, time_unspecced=%ld, "
      "frags=%ld, oldfrags=%ld, numb_deaths=%lu, "
      "condition_0=%d, condition_1=%d, condition_2=%d, condition_3=%d, condition_4=%d, "
      "poof_in='%s', poof_out='%s', poof_in_sound='%s', poof_out_sound='%s', "
      "echo_toggle=%d, prompt=%d, wiz_invis=%d, law_flags=%lu, "
      "wimpy=%d, aggressive=%d, highest_level=%d, screen_length=%d, "
      "quest_active=%d, quest_mob_vnum=%d, quest_type=%d, quest_accomplished=%d, "
      "quest_started=%d, quest_zone_number=%d, quest_giver=%d, quest_level=%d, "
      "quest_receiver=%d, quest_shares_left=%d, quest_kill_how_many=%d, "
      "quest_kill_original=%d, quest_map_room=%d, quest_map_bought=%d, "
      "last_ip=%lu "
      "WHERE pid=%d",
      esc_short, esc_long, esc_desc, esc_title,
      ch->player.m_class, ch->player.secondary_class, ch->player.spec, GET_RACE(ch), GET_RACEWAR(ch),
      GET_LEVEL(ch), GET_SEX(ch), ch->player.weight, ch->player.height, GET_SIZE(ch),
      GET_HOME(ch), GET_BIRTHPLACE(ch), GET_ORIG_BIRTHPLACE(ch), room,
      ch->player.time.birth, ch->player.time.played, (long)time(0), ch->player.time.perm_aging,
      ch->base_stats.Str, ch->base_stats.Dex, ch->base_stats.Agi, ch->base_stats.Con, ch->base_stats.Pow,
      ch->base_stats.Int, ch->base_stats.Wis, ch->base_stats.Cha, ch->base_stats.Kar, ch->base_stats.Luk,
      GET_MANA(ch), ch->points.base_mana, MAX(0, GET_MAX_HIT(ch) - GET_HIT(ch)), ch->points.base_hit,
      GET_VITALITY(ch), ch->points.base_vitality, ch->only.pc->spells_memmed[MAX_CIRCLE],
      GET_COPPER(ch), GET_SILVER(ch), GET_GOLD(ch), GET_PLATINUM(ch),
      GET_BALANCE_COPPER(ch), GET_BALANCE_SILVER(ch), GET_BALANCE_GOLD(ch), GET_BALANCE_PLATINUM(ch),
      GET_EXP(ch), ch->only.pc->epics, ch->only.pc->epic_skill_points, ch->only.pc->skillpoints, ch->only.pc->spell_bind_used,
      ch->specials.act, ch->specials.act2, ch->only.pc->vote, ch->specials.alignment,
      ch->only.pc->prestige, GET_ASSOC_ID(ch), ch->specials.guild_status,
      ch->only.pc->time_left_guild, ch->only.pc->nb_left_guild, ch->only.pc->time_unspecced,
      ch->only.pc->frags, ch->only.pc->oldfrags, ch->only.pc->numb_deaths,
      ch->specials.conditions[0], ch->specials.conditions[1], ch->specials.conditions[2],
      ch->specials.conditions[3], ch->specials.conditions[4],
      esc_poofin, esc_poofout, esc_poofinsnd, esc_poofoutsnd,
      ch->only.pc->echo_toggle, ch->only.pc->prompt, ch->only.pc->wiz_invis, ch->only.pc->law_flags,
      ch->only.pc->wimpy, ch->only.pc->aggressive, ch->only.pc->highest_level, ch->only.pc->screen_length,
      ch->only.pc->quest_active, ch->only.pc->quest_mob_vnum, ch->only.pc->quest_type, ch->only.pc->quest_accomplished,
      ch->only.pc->quest_started, ch->only.pc->quest_zone_number, ch->only.pc->quest_giver, ch->only.pc->quest_level,
      ch->only.pc->quest_receiver, ch->only.pc->quest_shares_left, ch->only.pc->quest_kill_how_many,
      ch->only.pc->quest_kill_original, ch->only.pc->quest_map_room, ch->only.pc->quest_map_bought,
      ch->only.pc->last_ip,
      pid
    );
  }
  else
  {
    // insert new player
    written = snprintf(q, remaining,
      "INSERT INTO player_data ("
      "name, short_descr, long_descr, description, title, "
      "m_class, secondary_class, spec, race, racewar, level, sex, "
      "weight, height, size, hometown, birthplace, orig_birthplace, last_room, "
      "birth_time, played_time, last_save, perm_aging, "
      "base_str, base_dex, base_agi, base_con, base_pow, "
      "base_int, base_wis, base_cha, base_kar, base_luk, "
      "mana, base_mana, hit_diff, base_hit, vitality, base_vitality, spells_memmed_extra, "
      "copper, silver, gold, platinum, bank_copper, bank_silver, bank_gold, bank_platinum, "
      "exp, epics, epic_skill_points, skillpoints, spell_bind_used, "
      "act, act2, vote, alignment, "
      "prestige, assoc_id, guild_status, time_left_guild, nb_left_guild, time_unspecced, "
      "frags, oldfrags, numb_deaths, "
      "condition_0, condition_1, condition_2, condition_3, condition_4, "
      "poof_in, poof_out, poof_in_sound, poof_out_sound, "
      "echo_toggle, prompt, wiz_invis, law_flags, wimpy, aggressive, highest_level, screen_length, "
      "quest_active, quest_mob_vnum, quest_type, quest_accomplished, "
      "quest_started, quest_zone_number, quest_giver, quest_level, "
      "quest_receiver, quest_shares_left, quest_kill_how_many, "
      "quest_kill_original, quest_map_room, quest_map_bought, last_ip"
      ") VALUES ("
      "'%s', '%s', '%s', '%s', '%s', "
      "%u, %u, %d, %d, %d, %d, %d, "
      "%d, %d, %d, %d, %d, %d, %d, "
      "%ld, %d, %ld, %d, "
      "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, "
      "%d, %d, %d, %d, %d, %d, %d, "
      "%d, %d, %d, %d, %d, %d, %d, %d, "
      "%d, %ld, %ld, %d, %ld, "
      "%u, %u, %lu, %d, "
      "%d, %d, %u, %ld, %d, %ld, "
      "%ld, %ld, %lu, "
      "%d, %d, %d, %d, %d, "
      "'%s', '%s', '%s', '%s', "
      "%d, %d, %d, %lu, %d, %d, %d, %d, "
      "%d, %d, %d, %d, "
      "%d, %d, %d, %d, "
      "%d, %d, %d, "
      "%d, %d, %d, %lu"
      ")",
      esc_name, esc_short, esc_long, esc_desc, esc_title,
      ch->player.m_class, ch->player.secondary_class, ch->player.spec, GET_RACE(ch), GET_RACEWAR(ch),
      GET_LEVEL(ch), GET_SEX(ch),
      ch->player.weight, ch->player.height, GET_SIZE(ch),
      GET_HOME(ch), GET_BIRTHPLACE(ch), GET_ORIG_BIRTHPLACE(ch), room,
      ch->player.time.birth, ch->player.time.played, (long)time(0), ch->player.time.perm_aging,
      ch->base_stats.Str, ch->base_stats.Dex, ch->base_stats.Agi, ch->base_stats.Con, ch->base_stats.Pow,
      ch->base_stats.Int, ch->base_stats.Wis, ch->base_stats.Cha, ch->base_stats.Kar, ch->base_stats.Luk,
      GET_MANA(ch), ch->points.base_mana, MAX(0, GET_MAX_HIT(ch) - GET_HIT(ch)), ch->points.base_hit,
      GET_VITALITY(ch), ch->points.base_vitality, ch->only.pc->spells_memmed[MAX_CIRCLE],
      GET_COPPER(ch), GET_SILVER(ch), GET_GOLD(ch), GET_PLATINUM(ch),
      GET_BALANCE_COPPER(ch), GET_BALANCE_SILVER(ch), GET_BALANCE_GOLD(ch), GET_BALANCE_PLATINUM(ch),
      GET_EXP(ch), ch->only.pc->epics, ch->only.pc->epic_skill_points, ch->only.pc->skillpoints, ch->only.pc->spell_bind_used,
      ch->specials.act, ch->specials.act2, ch->only.pc->vote, ch->specials.alignment,
      ch->only.pc->prestige, GET_ASSOC_ID(ch), ch->specials.guild_status,
      ch->only.pc->time_left_guild, ch->only.pc->nb_left_guild, ch->only.pc->time_unspecced,
      ch->only.pc->frags, ch->only.pc->oldfrags, ch->only.pc->numb_deaths,
      ch->specials.conditions[0], ch->specials.conditions[1], ch->specials.conditions[2],
      ch->specials.conditions[3], ch->specials.conditions[4],
      esc_poofin, esc_poofout, esc_poofinsnd, esc_poofoutsnd,
      ch->only.pc->echo_toggle, ch->only.pc->prompt, ch->only.pc->wiz_invis, ch->only.pc->law_flags,
      ch->only.pc->wimpy, ch->only.pc->aggressive, ch->only.pc->highest_level, ch->only.pc->screen_length,
      ch->only.pc->quest_active, ch->only.pc->quest_mob_vnum, ch->only.pc->quest_type, ch->only.pc->quest_accomplished,
      ch->only.pc->quest_started, ch->only.pc->quest_zone_number, ch->only.pc->quest_giver, ch->only.pc->quest_level,
      ch->only.pc->quest_receiver, ch->only.pc->quest_shares_left, ch->only.pc->quest_kill_how_many,
      ch->only.pc->quest_kill_original, ch->only.pc->quest_map_room, ch->only.pc->quest_map_bought,
      ch->only.pc->last_ip
    );
  }

  // free escaped strings
  free(esc_name);
  free(esc_short);
  free(esc_long);
  free(esc_desc);
  free(esc_title);
  free(esc_poofin);
  free(esc_poofout);
  free(esc_poofinsnd);
  free(esc_poofoutsnd);

  // run the main query
  if (!sql_run_query(query))
  {
    sql_player_error("sql_save_player_status", query);
    return false;
  }

  // if insert, get the new pid
  if (!is_update)
  {
    ch->only.pc->pid = (int)mysql_insert_id(DB);
    pid = ch->only.pc->pid;
  }

  // now save the array data: languages, intros, timers, undead slots, forged items, granted cmds

  // languages
  char del_query[256];
  snprintf(del_query, sizeof(del_query), "DELETE FROM player_languages WHERE pid=%d", pid);
  sql_run_query(del_query);

  for (int i = 0; i < MAX_TONGUE; i++)
  {
    if (GET_LANGUAGE(ch, i) > 0)
    {
      char ins_query[256];
      snprintf(ins_query, sizeof(ins_query),
               "INSERT INTO player_languages (pid, tongue_id, proficiency) VALUES (%d, %d, %d)",
               pid, i, GET_LANGUAGE(ch, i));
      sql_run_query(ins_query);
    }
  }

  // intros
  snprintf(del_query, sizeof(del_query), "DELETE FROM player_intros WHERE pid=%d", pid);
  sql_run_query(del_query);

  for (int i = 0; i < MAX_INTRO; i++)
  {
    if (ch->only.pc->introd_list[i] != 0)
    {
      char ins_query[256];
      snprintf(ins_query, sizeof(ins_query),
               "INSERT INTO player_intros (pid, intro_index, intro_pid, intro_time) VALUES (%d, %d, %ld, %lu)",
               pid, i, ch->only.pc->introd_list[i], ch->only.pc->introd_times[i]);
      sql_run_query(ins_query);
    }
  }

  // timers
  snprintf(del_query, sizeof(del_query), "DELETE FROM player_timers WHERE pid=%d", pid);
  sql_run_query(del_query);

  for (int i = 0; i < NUMB_PC_TIMERS; i++)
  {
    if (ch->only.pc->pc_timer[i] != 0)
    {
      char ins_query[256];
      snprintf(ins_query, sizeof(ins_query),
               "INSERT INTO player_timers (pid, timer_id, timer_value) VALUES (%d, %d, %ld)",
               pid, i, (long)ch->only.pc->pc_timer[i]);
      sql_run_query(ins_query);
    }
  }

  // undead spell slots
  snprintf(del_query, sizeof(del_query), "DELETE FROM player_undead_slots WHERE pid=%d", pid);
  sql_run_query(del_query);

  for (int i = 0; i <= MAX_CIRCLE; i++)
  {
    if (ch->specials.undead_spell_slots[i] != 0)
    {
      char ins_query[256];
      snprintf(ins_query, sizeof(ins_query),
               "INSERT INTO player_undead_slots (pid, circle, slots) VALUES (%d, %d, %d)",
               pid, i, ch->specials.undead_spell_slots[i]);
      sql_run_query(ins_query);
    }
  }

  // forged items
  snprintf(del_query, sizeof(del_query), "DELETE FROM player_forged_items WHERE pid=%d", pid);
  sql_run_query(del_query);

  for (int i = 0; i < MAX_FORGE_ITEMS; i++)
  {
    if (ch->only.pc->learned_forged_list[i] != 0)
    {
      char ins_query[256];
      snprintf(ins_query, sizeof(ins_query),
               "INSERT INTO player_forged_items (pid, forge_index, item_vnum) VALUES (%d, %d, %ld)",
               pid, i, ch->only.pc->learned_forged_list[i]);
      sql_run_query(ins_query);
    }
  }

  // granted commands
  snprintf(del_query, sizeof(del_query), "DELETE FROM player_granted_cmds WHERE pid=%d", pid);
  sql_run_query(del_query);

  for (int i = 0; i < ch->only.pc->numb_gcmd; i++)
  {
    char ins_query[256];
    snprintf(ins_query, sizeof(ins_query),
             "INSERT INTO player_granted_cmds (pid, cmd_num) VALUES (%d, %d)",
             pid, ch->only.pc->gcmd_arr[i]);
    sql_run_query(ins_query);
  }

  return true;
}

// ============================================================================
// skills save
// ============================================================================

bool sql_save_player_skills(P_char ch)
{
  if (!ch || !IS_PC(ch) || !DB)
    return false;

  int pid = GET_PID(ch);
  if (pid <= 0)
    return false;

  // delete existing skills
  char del_query[128];
  snprintf(del_query, sizeof(del_query), "DELETE FROM player_skills WHERE pid=%d", pid);
  sql_run_query(del_query);

  // insert non-zero skills
  for (int i = 0; i < MAX_SKILLS; i++)
  {
    if (ch->only.pc->skills[i].learned > 0 || ch->only.pc->skills[i].taught > 0)
    {
      char ins_query[256];
      snprintf(ins_query, sizeof(ins_query),
               "INSERT INTO player_skills (pid, skill_id, learned, taught) VALUES (%d, %d, %d, %d)",
               pid, i, ch->only.pc->skills[i].learned, ch->only.pc->skills[i].taught);
      sql_run_query(ins_query);
    }
  }

  return true;
}

// ============================================================================
// affects save
// ============================================================================

bool sql_save_player_affects(P_char ch)
{
  if (!ch || !IS_PC(ch) || !DB)
    return false;

  int pid = GET_PID(ch);
  if (pid <= 0)
    return false;

  // delete existing affects
  char del_query[128];
  snprintf(del_query, sizeof(del_query), "DELETE FROM player_affects WHERE pid=%d", pid);
  sql_run_query(del_query);

  // insert current affects
  for (struct affected_type *af = ch->affected; af; af = af->next)
  {
    // skip nosave affects
    if (IS_SET(af->flags, AFFTYPE_NOSAVE))
      continue;

    // todo: handle custom messages if needed
    char ins_query[1024];
    snprintf(ins_query, sizeof(ins_query),
             "INSERT INTO player_affects (pid, type, duration, flags, modifier, location, level, "
             "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5) "
             "VALUES (%d, %d, %d, %d, %d, %d, %d, %lu, %lu, %lu, %lu, %lu)",
             pid, af->type, af->duration, af->flags, af->modifier, af->location, af->level,
             af->bitvector, af->bitvector2, af->bitvector3, af->bitvector4, af->bitvector5);
    sql_run_query(ins_query);
  }

  return true;
}

// ============================================================================
// items save
// ============================================================================

// save item affects (the obj->affected[] array)
static bool sql_save_item_affects(int item_id, P_obj obj)
{
  for (int i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    if (obj->affected[i].location != 0 || obj->affected[i].modifier != 0)
    {
      char ins_query[256];
      snprintf(ins_query, sizeof(ins_query),
               "INSERT INTO player_item_affects (item_id, location, modifier) VALUES (%d, %d, %d)",
               item_id, obj->affected[i].location, obj->affected[i].modifier);
      if (!sql_run_query(ins_query))
        return false;
    }
  }
  return true;
}

// save a single item and its contents recursively
// returns the item_id of the inserted item, or 0 on failure
static int sql_save_single_item_get_id(int pid, P_obj obj, int equip_slot, int container_id)
{
  if (!obj || !DB)
    return 0;

  // skip norent items
  if (IS_SET(obj->extra_flags, ITEM_NORENT))
    return 0;

  int vnum = obj_index[obj->R_num].virtual_number;

  // escape strings - only save if strung (different from prototype)
  // STRUNG_KEYS = name, STRUNG_DESC2 = short_description,
  // STRUNG_DESC1 = description, STRUNG_DESC3 = action_description
  char *esc_name = NULL;
  char *esc_short = NULL;
  char *esc_desc = NULL;
  char *esc_action = NULL;

  if (obj->str_mask & STRUNG_KEYS)
    esc_name = sql_escape_string(obj->name ? obj->name : "");
  if (obj->str_mask & STRUNG_DESC2)
    esc_short = sql_escape_string(obj->short_description ? obj->short_description : "");
  if (obj->str_mask & STRUNG_DESC1)
    esc_desc = sql_escape_string(obj->description ? obj->description : "");
  if (obj->str_mask & STRUNG_DESC3)
    esc_action = sql_escape_string(obj->action_description ? obj->action_description : "");

  // build container_id string
  char container_str[32];
  if (container_id > 0)
    snprintf(container_str, sizeof(container_str), "%d", container_id);
  else
    strcpy(container_str, "NULL");

  // build name string with quotes or NULL
  char name_str[1024];
  if (esc_name)
    snprintf(name_str, sizeof(name_str), "'%s'", esc_name);
  else
    strcpy(name_str, "NULL");

  char short_str[1024];
  if (esc_short)
    snprintf(short_str, sizeof(short_str), "'%s'", esc_short);
  else
    strcpy(short_str, "NULL");

  char desc_str[2048];
  if (esc_desc)
    snprintf(desc_str, sizeof(desc_str), "'%s'", esc_desc);
  else
    strcpy(desc_str, "NULL");

  char action_str[2048];
  if (esc_action)
    snprintf(action_str, sizeof(action_str), "'%s'", esc_action);
  else
    strcpy(action_str, "NULL");

  // build the query - only use columns that exist in schema
  // extra fields like bitvectors, trap data, etc can be loaded from prototype
  char query[8192];
  snprintf(query, sizeof(query),
    "INSERT INTO player_items ("
    "pid, vnum, equip_slot, container_id, quantity, "
    "weight, cost, timer, extra_flags, "
    "value0, value1, value2, value3, value4, value5, value6, value7, "
    "name, short_descr, description, action_descr"
    ") VALUES ("
    "%d, %d, %d, %s, 1, "
    "%d, %d, %ld, %u, "
    "%d, %d, %d, %d, %d, %d, %d, %d, "
    "%s, %s, %s, %s"
    ")",
    pid, vnum, equip_slot, container_str,
    obj->weight, obj->cost, (long)obj->timer[0], obj->extra_flags,
    obj->value[0], obj->value[1], obj->value[2], obj->value[3],
    obj->value[4], obj->value[5], obj->value[6], obj->value[7],
    name_str, short_str, desc_str, action_str
  );

  // free escaped strings
  if (esc_name) free(esc_name);
  if (esc_short) free(esc_short);
  if (esc_desc) free(esc_desc);
  if (esc_action) free(esc_action);

  if (!sql_run_query(query))
  {
    sql_player_error("sql_save_single_item", query);
    return 0;
  }

  // get the inserted item_id
  int item_id = (int)mysql_insert_id(DB);

  // save item affects
  if (!sql_save_item_affects(item_id, obj))
    return 0;

  // recursively save container contents
  if (obj->contains)
  {
    for (P_obj content = obj->contains; content; content = content->next_content)
    {
      if (!IS_SET(content->extra_flags, ITEM_NORENT))
      {
        if (sql_save_single_item_get_id(pid, content, 0, item_id) == 0)
        {
          // log but continue - don't fail the whole save for one bad item
          logit(LOG_DEBUG, "sql_save_player_items: failed to save container content vnum %d",
                obj_index[content->R_num].virtual_number);
        }
      }
    }
  }

  return item_id;
}

bool sql_save_player_items(P_char ch)
{
  if (!ch || !IS_PC(ch) || !DB)
    return false;

  int pid = GET_PID(ch);
  if (pid <= 0)
    return false;

  // delete existing items (cascade deletes item_affects too)
  char del_query[128];
  snprintf(del_query, sizeof(del_query), "DELETE FROM player_items WHERE pid=%d", pid);
  sql_run_query(del_query);

  // save equipment (slots 1-42, 0 is special)
  for (int i = 0; i < MAX_WEAR; i++)
  {
    if (ch->equipment[i])
    {
      if (!IS_SET(ch->equipment[i]->extra_flags, ITEM_NORENT))
      {
        if (sql_save_single_item_get_id(pid, ch->equipment[i], i + 1, 0) == 0)
        {
          logit(LOG_DEBUG, "sql_save_player_items: failed to save equipment slot %d for %s",
                i, GET_NAME(ch));
        }
      }
    }
  }

  // save inventory (equip_slot = 0)
  for (P_obj obj = ch->carrying; obj; obj = obj->next_content)
  {
    if (!IS_SET(obj->extra_flags, ITEM_NORENT))
    {
      if (sql_save_single_item_get_id(pid, obj, 0, 0) == 0)
      {
        logit(LOG_DEBUG, "sql_save_player_items: failed to save inventory item for %s",
              GET_NAME(ch));
      }
    }
  }

  return true;
}

// ============================================================================
// witnesses save
// ============================================================================

bool sql_save_player_witnesses(P_char ch)
{
  if (!ch || !IS_PC(ch) || !DB)
    return false;

  int pid = GET_PID(ch);
  if (pid <= 0)
    return false;

  // delete existing witnesses
  char del_query[128];
  snprintf(del_query, sizeof(del_query), "DELETE FROM player_witnesses WHERE pid=%d", pid);
  sql_run_query(del_query);

  // insert current witnesses
  for (wtns_rec *w = ch->specials.witnessed; w; w = w->next)
  {
    char *esc_attacker = sql_escape_string(w->attacker ? w->attacker : "");
    char *esc_victim = sql_escape_string(w->victim ? w->victim : "");
    char ins_query[512];
    snprintf(ins_query, sizeof(ins_query),
             "INSERT INTO player_witnesses (pid, crime, room_vnum, attacker_name, victim_name, witness_time) "
             "VALUES (%d, %d, %d, '%s', '%s', %ld)",
             pid, w->crime, w->room, esc_attacker ? esc_attacker : "", esc_victim ? esc_victim : "", (long)w->time);
    free(esc_attacker);
    free(esc_victim);
    sql_run_query(ins_query);
  }

  return true;
}

// ============================================================================
// player load functions
// ============================================================================

// helper to safely get int from row, returns default if null
static int sql_row_int(MYSQL_ROW row, int idx, int def)
{
  return (row && row[idx]) ? atoi(row[idx]) : def;
}

// helper to safely get long from row
static long sql_row_long(MYSQL_ROW row, int idx, long def)
{
  return (row && row[idx]) ? atol(row[idx]) : def;
}

// helper to safely get ulong from row
static unsigned long sql_row_ulong(MYSQL_ROW row, int idx, unsigned long def)
{
  return (row && row[idx]) ? strtoul(row[idx], NULL, 10) : def;
}

// helper to duplicate string from row (caller must free)
static char *sql_row_str(MYSQL_ROW row, int idx)
{
  if (!row || !row[idx])
    return NULL;
  return strdup(row[idx]);
}

bool sql_load_player_status(P_char ch, int pid)
{
  if (!ch || !DB || pid <= 0)
    return false;

  char query[512];
  snprintf(query, sizeof(query),
    "SELECT name, short_descr, long_descr, description, title, "
    "m_class, secondary_class, spec, race, racewar, level, sex, "
    "weight, height, size, hometown, birthplace, orig_birthplace, last_room, "
    "birth_time, played_time, last_save, perm_aging, "
    "base_str, base_dex, base_agi, base_con, base_pow, "
    "base_int, base_wis, base_cha, base_kar, base_luk, "
    "mana, base_mana, hit_diff, base_hit, vitality, base_vitality, spells_memmed_extra, "
    "copper, silver, gold, platinum, bank_copper, bank_silver, bank_gold, bank_platinum, "
    "exp, epics, epic_skill_points, skillpoints, spell_bind_used, "
    "act, act2, vote, alignment, prestige, assoc_id, guild_status, "
    "time_left_guild, nb_left_guild, time_unspecced, frags, oldfrags, numb_deaths, "
    "condition_0, condition_1, condition_2, condition_3, condition_4, "
    "poof_in, poof_out, poof_in_sound, poof_out_sound, "
    "echo_toggle, prompt, wiz_invis, law_flags, wimpy, aggressive, highest_level, screen_length, "
    "quest_active, quest_mob_vnum, quest_type, quest_accomplished, "
    "quest_started, quest_zone_number, quest_giver, quest_level, "
    "quest_receiver, quest_shares_left, quest_kill_how_many, "
    "quest_kill_original, quest_map_room, quest_map_bought, last_ip "
    "FROM player_data WHERE pid=%d", pid);

  MYSQL_RES *result = db_query("%s", query);
  if (!result)
    return false;

  MYSQL_ROW row = mysql_fetch_row(result);
  if (!row)
  {
    mysql_free_result(result);
    return false;
  }

  int col = 0;

  // name and descriptions
  GET_NAME(ch) = sql_row_str(row, col++);
  ch->player.short_descr = sql_row_str(row, col++);
  ch->player.long_descr = sql_row_str(row, col++);
  ch->player.description = sql_row_str(row, col++);
  GET_TITLE(ch) = sql_row_str(row, col++);

  // class/race/level
  ch->player.m_class = sql_row_int(row, col++, 0);
  ch->player.secondary_class = sql_row_int(row, col++, 0);
  ch->player.spec = sql_row_int(row, col++, 0);
  GET_RACE(ch) = sql_row_int(row, col++, 0);
  GET_RACEWAR(ch) = sql_row_int(row, col++, 0);
  ch->player.level = sql_row_int(row, col++, 1);
  GET_SEX(ch) = sql_row_int(row, col++, 0);

  // physical
  ch->player.weight = sql_row_int(row, col++, 0);
  ch->player.height = sql_row_int(row, col++, 0);
  GET_SIZE(ch) = sql_row_int(row, col++, 0);

  // location
  GET_HOME(ch) = sql_row_int(row, col++, 0);
  GET_BIRTHPLACE(ch) = sql_row_int(row, col++, 0);
  GET_ORIG_BIRTHPLACE(ch) = sql_row_int(row, col++, 0);
  ch->in_room = real_room(sql_row_int(row, col++, 0));

  // time
  ch->player.time.birth = sql_row_long(row, col++, 0);
  ch->player.time.played = sql_row_int(row, col++, 0);
  ch->player.time.saved = sql_row_long(row, col++, 0);
  ch->player.time.logon = time(0);
  ch->player.time.perm_aging = sql_row_int(row, col++, 0);

  // base stats
  ch->base_stats.Str = sql_row_int(row, col++, 0);
  ch->base_stats.Dex = sql_row_int(row, col++, 0);
  ch->base_stats.Agi = sql_row_int(row, col++, 0);
  ch->base_stats.Con = sql_row_int(row, col++, 0);
  ch->base_stats.Pow = sql_row_int(row, col++, 0);
  ch->base_stats.Int = sql_row_int(row, col++, 0);
  ch->base_stats.Wis = sql_row_int(row, col++, 0);
  ch->base_stats.Cha = sql_row_int(row, col++, 0);
  ch->base_stats.Kar = sql_row_int(row, col++, 0);
  ch->base_stats.Luk = sql_row_int(row, col++, 0);

  // points
  GET_MANA(ch) = sql_row_int(row, col++, 0);
  ch->points.base_mana = sql_row_int(row, col++, 0);
  int hit_diff = sql_row_int(row, col++, 0);
  ch->points.base_hit = sql_row_int(row, col++, 0);
  GET_VITALITY(ch) = sql_row_int(row, col++, 0);
  ch->points.base_vitality = sql_row_int(row, col++, 0);
  ch->only.pc->spells_memmed[MAX_CIRCLE] = sql_row_int(row, col++, 0);

  // money
  GET_COPPER(ch) = sql_row_int(row, col++, 0);
  GET_SILVER(ch) = sql_row_int(row, col++, 0);
  GET_GOLD(ch) = sql_row_int(row, col++, 0);
  GET_PLATINUM(ch) = sql_row_int(row, col++, 0);
  GET_BALANCE_COPPER(ch) = sql_row_int(row, col++, 0);
  GET_BALANCE_SILVER(ch) = sql_row_int(row, col++, 0);
  GET_BALANCE_GOLD(ch) = sql_row_int(row, col++, 0);
  GET_BALANCE_PLATINUM(ch) = sql_row_int(row, col++, 0);

  // experience
  GET_EXP(ch) = sql_row_int(row, col++, 0);
  ch->only.pc->epics = sql_row_long(row, col++, 0);
  ch->only.pc->epic_skill_points = sql_row_long(row, col++, 0);
  ch->only.pc->skillpoints = sql_row_int(row, col++, 0);
  ch->only.pc->spell_bind_used = sql_row_long(row, col++, 0);

  // flags
  ch->specials.act = sql_row_ulong(row, col++, 0);
  ch->specials.act2 = sql_row_ulong(row, col++, 0);
  ch->only.pc->vote = sql_row_ulong(row, col++, 0);
  ch->specials.alignment = sql_row_int(row, col++, 0);
  ch->only.pc->prestige = sql_row_int(row, col++, 0);
  col++; // skip assoc_id - handled by assoc system on login
  ch->specials.guild_status = sql_row_int(row, col++, 0);
  ch->only.pc->time_left_guild = sql_row_long(row, col++, 0);
  ch->only.pc->nb_left_guild = sql_row_int(row, col++, 0);
  ch->only.pc->time_unspecced = sql_row_long(row, col++, 0);
  ch->only.pc->frags = sql_row_long(row, col++, 0);
  ch->only.pc->oldfrags = sql_row_long(row, col++, 0);
  ch->only.pc->numb_deaths = sql_row_ulong(row, col++, 0);

  // conditions
  ch->specials.conditions[0] = sql_row_int(row, col++, 0);
  ch->specials.conditions[1] = sql_row_int(row, col++, 0);
  ch->specials.conditions[2] = sql_row_int(row, col++, 0);
  ch->specials.conditions[3] = sql_row_int(row, col++, 0);
  ch->specials.conditions[4] = sql_row_int(row, col++, 0);

  // immortal stuff
  ch->only.pc->poofIn = sql_row_str(row, col++);
  ch->only.pc->poofOut = sql_row_str(row, col++);
  ch->only.pc->poofInSound = sql_row_str(row, col++);
  ch->only.pc->poofOutSound = sql_row_str(row, col++);
  ch->only.pc->echo_toggle = sql_row_int(row, col++, 0);
  ch->only.pc->prompt = sql_row_int(row, col++, 0);
  ch->only.pc->wiz_invis = sql_row_long(row, col++, 0);
  ch->only.pc->law_flags = sql_row_ulong(row, col++, 0);
  ch->only.pc->wimpy = sql_row_int(row, col++, 0);
  ch->only.pc->aggressive = sql_row_int(row, col++, -1);
  ch->only.pc->highest_level = sql_row_int(row, col++, 0);
  ch->only.pc->screen_length = sql_row_int(row, col++, 24);

  // quest data
  ch->only.pc->quest_active = sql_row_int(row, col++, 0);
  ch->only.pc->quest_mob_vnum = sql_row_int(row, col++, 0);
  ch->only.pc->quest_type = sql_row_int(row, col++, 0);
  ch->only.pc->quest_accomplished = sql_row_int(row, col++, 0);
  ch->only.pc->quest_started = sql_row_int(row, col++, 0);
  ch->only.pc->quest_zone_number = sql_row_int(row, col++, 0);
  ch->only.pc->quest_giver = sql_row_int(row, col++, 0);
  ch->only.pc->quest_level = sql_row_int(row, col++, 0);
  ch->only.pc->quest_receiver = sql_row_int(row, col++, 0);
  ch->only.pc->quest_shares_left = sql_row_int(row, col++, 0);
  ch->only.pc->quest_kill_how_many = sql_row_int(row, col++, 0);
  ch->only.pc->quest_kill_original = sql_row_int(row, col++, 0);
  ch->only.pc->quest_map_room = sql_row_int(row, col++, 0);
  ch->only.pc->quest_map_bought = sql_row_int(row, col++, 0);
  ch->only.pc->last_ip = sql_row_ulong(row, col++, 0);

  mysql_free_result(result);

  // set pid
  ch->only.pc->pid = pid;

  // calculate hit from hit_diff
  GET_HIT(ch) = GET_MAX_HIT(ch) - hit_diff;

  // load array data: languages, intros, timers, undead slots, forged items, granted cmds

  // languages
  snprintf(query, sizeof(query),
    "SELECT tongue_id, proficiency FROM player_languages WHERE pid=%d", pid);
  result = db_query("%s", query);
  if (result)
  {
    while ((row = mysql_fetch_row(result)))
    {
      int tongue = sql_row_int(row, 0, 0);
      if (tongue >= 0 && tongue < MAX_TONGUE)
        GET_LANGUAGE(ch, tongue) = sql_row_int(row, 1, 0);
    }
    mysql_free_result(result);
  }

  // intros
  snprintf(query, sizeof(query),
    "SELECT intro_index, intro_pid, intro_time FROM player_intros WHERE pid=%d", pid);
  result = db_query("%s", query);
  if (result)
  {
    while ((row = mysql_fetch_row(result)))
    {
      int idx = sql_row_int(row, 0, 0);
      if (idx >= 0 && idx < MAX_INTRO)
      {
        ch->only.pc->introd_list[idx] = sql_row_long(row, 1, 0);
        ch->only.pc->introd_times[idx] = sql_row_ulong(row, 2, 0);
      }
    }
    mysql_free_result(result);
  }

  // timers
  snprintf(query, sizeof(query),
    "SELECT timer_id, timer_value FROM player_timers WHERE pid=%d", pid);
  result = db_query("%s", query);
  if (result)
  {
    while ((row = mysql_fetch_row(result)))
    {
      int idx = sql_row_int(row, 0, 0);
      if (idx >= 0 && idx < NUMB_PC_TIMERS)
        ch->only.pc->pc_timer[idx] = sql_row_long(row, 1, 0);
    }
    mysql_free_result(result);
  }

  // undead slots
  snprintf(query, sizeof(query),
    "SELECT circle, slots FROM player_undead_slots WHERE pid=%d", pid);
  result = db_query("%s", query);
  if (result)
  {
    while ((row = mysql_fetch_row(result)))
    {
      int circle = sql_row_int(row, 0, 0);
      if (circle >= 0 && circle <= MAX_CIRCLE)
        ch->specials.undead_spell_slots[circle] = sql_row_int(row, 1, 0);
    }
    mysql_free_result(result);
  }

  // forged items
  snprintf(query, sizeof(query),
    "SELECT forge_index, item_vnum FROM player_forged_items WHERE pid=%d", pid);
  result = db_query("%s", query);
  if (result)
  {
    while ((row = mysql_fetch_row(result)))
    {
      int idx = sql_row_int(row, 0, 0);
      if (idx >= 0 && idx < MAX_FORGE_ITEMS)
        ch->only.pc->learned_forged_list[idx] = sql_row_long(row, 1, 0);
    }
    mysql_free_result(result);
  }

  // granted commands - count first, then allocate and load
  snprintf(query, sizeof(query),
    "SELECT COUNT(*) FROM player_granted_cmds WHERE pid=%d", pid);
  result = db_query("%s", query);
  if (result)
  {
    row = mysql_fetch_row(result);
    int cmd_count = sql_row_int(row, 0, 0);
    mysql_free_result(result);

    if (cmd_count > 0)
    {
      ch->only.pc->gcmd_arr = (int *)malloc(cmd_count * sizeof(int));
      if (ch->only.pc->gcmd_arr)
      {
        ch->only.pc->numb_gcmd = 0;
        snprintf(query, sizeof(query),
          "SELECT cmd_num FROM player_granted_cmds WHERE pid=%d ORDER BY id", pid);
        result = db_query("%s", query);
        if (result)
        {
          while ((row = mysql_fetch_row(result)) && ch->only.pc->numb_gcmd < cmd_count)
          {
            ch->only.pc->gcmd_arr[ch->only.pc->numb_gcmd++] = sql_row_int(row, 0, 0);
          }
          mysql_free_result(result);
        }
      }
    }
  }

  return true;
}

bool sql_load_player_skills(P_char ch)
{
  if (!ch || !IS_PC(ch) || !DB)
    return false;

  int pid = GET_PID(ch);
  if (pid <= 0)
    return false;

  char query[256];
  snprintf(query, sizeof(query),
    "SELECT skill_id, learned, taught FROM player_skills WHERE pid=%d", pid);

  MYSQL_RES *result = db_query("%s", query);
  if (!result)
    return false;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)))
  {
    int skill_id = sql_row_int(row, 0, 0);
    if (skill_id >= 0 && skill_id < MAX_SKILLS)
    {
      ch->only.pc->skills[skill_id].learned = sql_row_int(row, 1, 0);
      ch->only.pc->skills[skill_id].taught = sql_row_int(row, 2, 0);
    }
  }
  mysql_free_result(result);

  return true;
}

bool sql_load_player_affects(P_char ch)
{
  if (!ch || !IS_PC(ch) || !DB)
    return false;

  int pid = GET_PID(ch);
  if (pid <= 0)
    return false;

  char query[512];
  snprintf(query, sizeof(query),
    "SELECT type, duration, flags, modifier, location, level, "
    "bitvector1, bitvector2, bitvector3, bitvector4, bitvector5 "
    "FROM player_affects WHERE pid=%d", pid);

  MYSQL_RES *result = db_query("%s", query);
  if (!result)
    return false;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)))
  {
    struct affected_type af;
    memset(&af, 0, sizeof(af));

    af.type = sql_row_int(row, 0, 0);
    af.duration = sql_row_int(row, 1, 0);
    af.flags = sql_row_int(row, 2, 0);
    af.modifier = sql_row_int(row, 3, 0);
    af.location = sql_row_int(row, 4, 0);
    af.level = sql_row_int(row, 5, 0);
    af.bitvector = sql_row_ulong(row, 6, 0);
    af.bitvector2 = sql_row_ulong(row, 7, 0);
    af.bitvector3 = sql_row_ulong(row, 8, 0);
    af.bitvector4 = sql_row_ulong(row, 9, 0);
    af.bitvector5 = sql_row_ulong(row, 10, 0);

    affect_to_char(ch, &af);
  }
  mysql_free_result(result);

  return true;
}

bool sql_load_player_items(P_char ch)
{
  if (!ch || !IS_PC(ch) || !DB)
    return false;

  int pid = GET_PID(ch);
  if (pid <= 0)
    return false;

  // first, load all items into a temp array indexed by db id
  // then resolve container relationships

  char query[1024];
  snprintf(query, sizeof(query),
    "SELECT id, vnum, equip_slot, container_id, "
    "weight, cost, timer, extra_flags, "
    "value0, value1, value2, value3, value4, value5, value6, value7, "
    "name, short_descr, description, action_descr "
    "FROM player_items WHERE pid=%d ORDER BY id", pid);

  MYSQL_RES *result = db_query("%s", query);
  if (!result)
    return false;

  // count rows
  int num_rows = mysql_num_rows(result);
  if (num_rows == 0)
  {
    mysql_free_result(result);
    return true; // no items is valid
  }

  // allocate temp arrays
  P_obj *items = (P_obj *)calloc(num_rows, sizeof(P_obj));
  int *item_ids = (int *)calloc(num_rows, sizeof(int));
  int *container_ids = (int *)calloc(num_rows, sizeof(int));
  int *equip_slots = (int *)calloc(num_rows, sizeof(int));

  int idx = 0;
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) && idx < num_rows)
  {
    int col = 0;
    int db_id = sql_row_int(row, col++, 0);
    int vnum = sql_row_int(row, col++, 0);
    int equip_slot = sql_row_int(row, col++, 0);
    int container_id = sql_row_int(row, col++, 0);

    // create object from prototype
    P_obj obj = read_object(vnum, VIRTUAL);
    if (!obj)
    {
      logit(LOG_DEBUG, "sql_load_player_items: failed to load vnum %d for %s", vnum, GET_NAME(ch));
      idx++;
      continue;
    }

    // override saved properties
    obj->weight = sql_row_int(row, col++, obj->weight);
    obj->cost = sql_row_int(row, col++, obj->cost);
    obj->timer[0] = sql_row_long(row, col++, obj->timer[0]);
    obj->extra_flags = sql_row_ulong(row, col++, obj->extra_flags);

    obj->value[0] = sql_row_int(row, col++, obj->value[0]);
    obj->value[1] = sql_row_int(row, col++, obj->value[1]);
    obj->value[2] = sql_row_int(row, col++, obj->value[2]);
    obj->value[3] = sql_row_int(row, col++, obj->value[3]);
    obj->value[4] = sql_row_int(row, col++, obj->value[4]);
    obj->value[5] = sql_row_int(row, col++, obj->value[5]);
    obj->value[6] = sql_row_int(row, col++, obj->value[6]);
    obj->value[7] = sql_row_int(row, col++, obj->value[7]);

    // strung strings (if not NULL, replace prototype)
    char *str_name = sql_row_str(row, col++);
    char *str_short = sql_row_str(row, col++);
    char *str_desc = sql_row_str(row, col++);
    char *str_action = sql_row_str(row, col++);

    if (str_name)
    {
      obj->name = str_name;
      obj->str_mask |= STRUNG_KEYS;
    }
    if (str_short)
    {
      obj->short_description = str_short;
      obj->str_mask |= STRUNG_DESC2;
    }
    if (str_desc)
    {
      obj->description = str_desc;
      obj->str_mask |= STRUNG_DESC1;
    }
    if (str_action)
    {
      obj->action_description = str_action;
      obj->str_mask |= STRUNG_DESC3;
    }

    items[idx] = obj;
    item_ids[idx] = db_id;
    container_ids[idx] = container_id;
    equip_slots[idx] = equip_slot;
    idx++;
  }
  mysql_free_result(result);

  int loaded_count = idx;

  // load item affects for each item
  for (int i = 0; i < loaded_count; i++)
  {
    if (!items[i])
      continue;

    snprintf(query, sizeof(query),
      "SELECT location, modifier FROM player_item_affects WHERE item_id=%d", item_ids[i]);
    result = db_query("%s", query);
    if (result)
    {
      int aff_idx = 0;
      while ((row = mysql_fetch_row(result)) && aff_idx < MAX_OBJ_AFFECT)
      {
        items[i]->affected[aff_idx].location = sql_row_int(row, 0, 0);
        items[i]->affected[aff_idx].modifier = sql_row_int(row, 1, 0);
        aff_idx++;
      }
      mysql_free_result(result);
    }
  }

  // now place items: first pass - put items in containers
  for (int i = 0; i < loaded_count; i++)
  {
    if (!items[i] || container_ids[i] == 0)
      continue;

    // find container
    for (int j = 0; j < loaded_count; j++)
    {
      if (item_ids[j] == container_ids[i] && items[j])
      {
        obj_to_obj(items[i], items[j]);
        break;
      }
    }
  }

  // second pass - put top-level items on character
  for (int i = 0; i < loaded_count; i++)
  {
    if (!items[i] || container_ids[i] != 0)
      continue;

    if (equip_slots[i] > 0 && equip_slots[i] <= MAX_WEAR)
    {
      // equipment slot (1-indexed in db, 0-indexed in array)
      int slot = equip_slots[i] - 1;
      if (!ch->equipment[slot])
        equip_char(ch, items[i], slot, 0);
      else
        obj_to_char(items[i], ch);
    }
    else
    {
      // inventory
      obj_to_char(items[i], ch);
    }
  }

  free(items);
  free(item_ids);
  free(container_ids);
  free(equip_slots);

  return true;
}

bool sql_load_player_witnesses(P_char ch)
{
  if (!ch || !IS_PC(ch) || !DB)
    return false;

  int pid = GET_PID(ch);
  if (pid <= 0)
    return false;

  char query[256];
  snprintf(query, sizeof(query),
    "SELECT crime, room_vnum, attacker_name, victim_name, witness_time "
    "FROM player_witnesses WHERE pid=%d", pid);

  MYSQL_RES *result = db_query("%s", query);
  if (!result)
    return false;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)))
  {
    wtns_rec *w = (wtns_rec *)malloc(sizeof(wtns_rec));
    if (!w)
      continue;

    memset(w, 0, sizeof(wtns_rec));
    w->crime = sql_row_int(row, 0, 0);
    w->room = sql_row_int(row, 1, 0);
    w->attacker = sql_row_str(row, 2);
    w->victim = sql_row_str(row, 3);
    w->time = sql_row_long(row, 4, 0);

    // prepend to list
    w->next = ch->specials.witnessed;
    ch->specials.witnessed = w;
  }
  mysql_free_result(result);

  return true;
}

P_char sql_load_player(const char *name)
{
  if (!name || !DB)
    return NULL;

  // get pid first
  int pid = sql_get_player_pid(name);
  if (pid <= 0)
  {
    logit(LOG_DEBUG, "sql_load_player: player %s not found in db", name);
    return NULL;
  }

  // allocate character structure
  P_char ch = (P_char)malloc(sizeof(struct char_data));
  if (!ch)
    return NULL;
  memset(ch, 0, sizeof(struct char_data));

  // allocate pc_only_data
  ch->only.pc = (struct pc_only_data *)malloc(sizeof(struct pc_only_data));
  if (!ch->only.pc)
  {
    free(ch);
    return NULL;
  }
  memset(ch->only.pc, 0, sizeof(struct pc_only_data));

  // IS_PC is defined as !IS_NPC, and IS_NPC checks ACT_ISNPC flag
  // since we memset to 0, the flag is not set, so this is already a PC

  // load all components
  if (!sql_load_player_status(ch, pid))
  {
    logit(LOG_DEBUG, "sql_load_player: failed to load status for %s", name);
    free(ch->only.pc);
    free(ch);
    return NULL;
  }

  if (!sql_load_player_skills(ch))
  {
    logit(LOG_DEBUG, "sql_load_player: failed to load skills for %s", name);
    // continue anyway, skills aren't fatal
  }

  if (!sql_load_player_affects(ch))
  {
    logit(LOG_DEBUG, "sql_load_player: failed to load affects for %s", name);
    // continue anyway
  }

  if (!sql_load_player_items(ch))
  {
    logit(LOG_DEBUG, "sql_load_player: failed to load items for %s", name);
    // continue anyway
  }

  if (!sql_load_player_witnesses(ch))
  {
    logit(LOG_DEBUG, "sql_load_player: failed to load witnesses for %s", name);
    // continue anyway
  }

  return ch;
}

// ============================================================================
// account functions - placeholder stubs
// ============================================================================

bool sql_save_account(struct acct_entry *acc)
{
  // todo: implement
  logit(LOG_DEBUG, "sql_save_account: not yet implemented");
  return false;
}

struct acct_entry *sql_load_account(const char *name)
{
  // todo: implement
  return NULL;
}

bool sql_account_exists(const char *name)
{
  if (!DB || !name)
    return false;

  char *escaped_name = sql_escape_string(name);
  if (!escaped_name)
    return false;

  char query[256];
  snprintf(query, sizeof(query),
           "SELECT 1 FROM accounts WHERE account_name='%s' LIMIT 1",
           escaped_name);
  free(escaped_name);

  MYSQL_RES *result = db_query("%s", query);
  if (!result)
    return false;

  MYSQL_ROW row = mysql_fetch_row(result);
  bool exists = (row != NULL);
  mysql_free_result(result);

  return exists;
}

bool sql_link_player_to_account(const char *account_name, int pid)
{
  // todo: implement
  return false;
}

// ============================================================================
// locker functions - placeholder stubs
// ============================================================================

bool sql_save_locker(P_char locker_ch, int owner_pid, int owner_assoc_id)
{
  // todo: implement
  logit(LOG_DEBUG, "sql_save_locker: not yet implemented");
  return false;
}

P_char sql_load_locker(int owner_pid, int owner_assoc_id)
{
  // todo: implement
  return NULL;
}

bool sql_locker_exists(int owner_pid, int owner_assoc_id)
{
  // todo: implement
  return false;
}

bool sql_delete_locker(int owner_pid, int owner_assoc_id)
{
  // todo: implement
  return false;
}

// ============================================================================
// migration helpers
// ============================================================================

// allocate a temp char for migration (uses malloc, not pools)
static P_char alloc_temp_char(void)
{
  P_char ch = (P_char)malloc(sizeof(struct char_data));
  if (!ch)
    return NULL;
  memset(ch, 0, sizeof(struct char_data));

  ch->only.pc = (struct pc_only_data *)malloc(sizeof(struct pc_only_data));
  if (!ch->only.pc)
  {
    free(ch);
    return NULL;
  }
  memset(ch->only.pc, 0, sizeof(struct pc_only_data));
  return ch;
}

// free a temp char allocated by alloc_temp_char
// also frees any items the char is carrying
static void free_temp_char(P_char ch)
{
  if (!ch)
    return;

  // free items (carrying and equipment)
  P_obj obj, next;
  for (obj = ch->carrying; obj; obj = next)
  {
    next = obj->next_content;
    free_obj(obj);
  }
  for (int i = 0; i < MAX_WEAR; i++)
  {
    if (ch->equipment[i])
      free_obj(ch->equipment[i]);
  }

  // free strings if allocated
  if (ch->player.name)
    free(ch->player.name);
  if (ch->player.short_descr)
    free(ch->player.short_descr);
  if (ch->player.long_descr)
    free(ch->player.long_descr);
  if (ch->player.description)
    free(ch->player.description);
  if (ch->player.title)
    free(ch->player.title);
  if (ch->only.pc && ch->only.pc->poofIn)
    free(ch->only.pc->poofIn);
  if (ch->only.pc && ch->only.pc->poofOut)
    free(ch->only.pc->poofOut);

  if (ch->only.pc)
    free(ch->only.pc);
  free(ch);
}

bool sql_migrate_player(const char *name)
{
  if (!name || !*name)
    return false;

  logit(LOG_DEBUG, "sql_migrate_player: migrating %s", name);

  // check if already in db
  if (sql_player_exists(name))
  {
    logit(LOG_DEBUG, "sql_migrate_player: %s already exists in db, skipping", name);
    return true;
  }

  // allocate temp char
  P_char ch = alloc_temp_char();
  if (!ch)
  {
    logit(LOG_FILE, "sql_migrate_player: failed to allocate char for %s", name);
    return false;
  }

  // load from pfile
  int status = restoreCharOnly(ch, (char *)name);
  if (status < 0)
  {
    logit(LOG_FILE, "sql_migrate_player: failed to load pfile for %s (status %d)", name, status);
    free_temp_char(ch);
    return false;
  }

  // load items
  ch->carrying = NULL;
  for (int i = 0; i < MAX_WEAR; i++)
    ch->equipment[i] = NULL;
  restoreItemsOnly(ch, 0);

  // save to db
  // use status as rent type, room 0 (will be fixed on login)
  bool result = sql_save_player(ch, status, 0);
  if (!result)
  {
    logit(LOG_FILE, "sql_migrate_player: failed to save %s to db", name);
    free_temp_char(ch);
    return false;
  }

  logit(LOG_DEBUG, "sql_migrate_player: successfully migrated %s", name);
  free_temp_char(ch);
  return true;
}

bool sql_verify_player(const char *name)
{
  if (!name || !*name)
    return false;

  // load from pfile
  P_char pfile_ch = alloc_temp_char();
  if (!pfile_ch)
    return false;

  int status = restoreCharOnly(pfile_ch, (char *)name);
  if (status < 0)
  {
    free_temp_char(pfile_ch);
    return false;
  }

  // load from db
  P_char db_ch = sql_load_player(name);
  if (!db_ch)
  {
    logit(LOG_FILE, "sql_verify_player: %s not found in db", name);
    free_temp_char(pfile_ch);
    return false;
  }

  // compare key fields
  bool match = true;

  if (strcmp(GET_NAME(pfile_ch), GET_NAME(db_ch)) != 0)
  {
    logit(LOG_FILE, "sql_verify_player: %s name mismatch", name);
    match = false;
  }
  if (GET_LEVEL(pfile_ch) != GET_LEVEL(db_ch))
  {
    logit(LOG_FILE, "sql_verify_player: %s level mismatch (%d vs %d)",
          name, GET_LEVEL(pfile_ch), GET_LEVEL(db_ch));
    match = false;
  }
  if (GET_RACE(pfile_ch) != GET_RACE(db_ch))
  {
    logit(LOG_FILE, "sql_verify_player: %s race mismatch", name);
    match = false;
  }
  if (pfile_ch->player.m_class != db_ch->player.m_class)
  {
    logit(LOG_FILE, "sql_verify_player: %s class mismatch", name);
    match = false;
  }
  if (GET_EXP(pfile_ch) != GET_EXP(db_ch))
  {
    logit(LOG_FILE, "sql_verify_player: %s exp mismatch (%ld vs %ld)",
          name, GET_EXP(pfile_ch), GET_EXP(db_ch));
    match = false;
  }
  if (GET_GOLD(pfile_ch) != GET_GOLD(db_ch))
  {
    logit(LOG_FILE, "sql_verify_player: %s gold mismatch", name);
    match = false;
  }

  free_temp_char(pfile_ch);
  free_temp_char(db_ch);

  if (match)
    logit(LOG_DEBUG, "sql_verify_player: %s verified OK", name);

  return match;
}

// migrate all players from pfiles to db
// returns count of successfully migrated players
int sql_migrate_all_players(void)
{
  DIR *pf_dir;
  struct dirent *pf_entry;
  char dname[256];
  char fname[256];
  char letter;
  char *dot_index;
  int success_count = 0;
  int fail_count = 0;
  int skip_count = 0;

  logit(LOG_DEBUG, "sql_migrate_all_players: starting migration");

  for (letter = 'a'; letter <= 'z'; letter++)
  {
    snprintf(dname, 256, "%s/%c", SAVE_DIR, letter);
    pf_dir = opendir(dname);
    if (!pf_dir)
      continue;

    while ((pf_entry = readdir(pf_dir)) != NULL)
    {
      strcpy(fname, pf_entry->d_name);

      // skip . and ..
      if (fname[0] == '.')
        continue;

      // skip files with extensions (like .locker, .old, etc)
      dot_index = strrchr(fname, '.');
      if (dot_index)
        continue;

      // try to migrate
      if (sql_player_exists(fname))
      {
        skip_count++;
        continue;
      }

      if (sql_migrate_player(fname))
        success_count++;
      else
        fail_count++;
    }

    closedir(pf_dir);
  }

  logit(LOG_DEBUG, "sql_migrate_all_players: done - %d migrated, %d failed, %d skipped",
        success_count, fail_count, skip_count);

  return success_count;
}

#endif // __NO_MYSQL__
