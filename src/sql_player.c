// sql_player.c
// player save/load functions for mysql storage
// part of pfile-to-db migration

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
// items save - placeholder, this is the complex one
// ============================================================================

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

  // todo: implement full item save with container hierarchy
  // this requires recursive traversal of equipment[] and carrying list
  // for now, just return true as a placeholder

  logit(LOG_DEBUG, "sql_save_player_items: not yet implemented for %s", GET_NAME(ch));
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
// player load - placeholder stubs
// ============================================================================

P_char sql_load_player(const char *name)
{
  // todo: implement full player load
  logit(LOG_DEBUG, "sql_load_player: not yet implemented for %s", name);
  return NULL;
}

bool sql_load_player_status(P_char ch, int pid)
{
  // todo: implement
  return false;
}

bool sql_load_player_skills(P_char ch)
{
  // todo: implement
  return false;
}

bool sql_load_player_affects(P_char ch)
{
  // todo: implement
  return false;
}

bool sql_load_player_items(P_char ch)
{
  // todo: implement
  return false;
}

bool sql_load_player_witnesses(P_char ch)
{
  // todo: implement
  return false;
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
// migration helpers - placeholder stubs
// ============================================================================

bool sql_migrate_player(const char *name)
{
  // todo: implement
  logit(LOG_DEBUG, "sql_migrate_player: not yet implemented for %s", name);
  return false;
}

bool sql_verify_player(const char *name)
{
  // todo: implement
  return false;
}

#endif // __NO_MYSQL__
