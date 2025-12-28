// sql_player.h
// player save/load functions for mysql storage
// part of pfile-to-db migration

#ifndef __SQL_PLAYER_H_INCLUDED__
#define __SQL_PLAYER_H_INCLUDED__

#include "structs.h"

#ifndef __NO_MYSQL__

// ============================================================================
// transaction helpers
// ============================================================================

// start a transaction, returns true on success
bool sql_begin_transaction(void);

// commit current transaction, returns true on success
bool sql_commit(void);

// rollback current transaction, returns true on success
bool sql_rollback(void);

// check if we're currently in a transaction
bool sql_in_transaction(void);

// ============================================================================
// player save functions
// ============================================================================

// master save function - saves entire player to db atomically
// type: save type (RENT_CAMPED, RENT_RENTED, etc from defines.h)
// room: room vnum to save
// returns true on success
bool sql_save_player(P_char ch, int type, int room);

// individual save functions (called by sql_save_player)
bool sql_save_player_status(P_char ch, int type, int room);
bool sql_save_player_skills(P_char ch);
bool sql_save_player_affects(P_char ch);
bool sql_save_player_items(P_char ch);
bool sql_save_player_witnesses(P_char ch);

// ============================================================================
// player load functions
// ============================================================================

// master load function - loads entire player from db
// name: player name to load
// returns char_data pointer or NULL on failure
P_char sql_load_player(const char *name);

// check if player exists in db
bool sql_player_exists(const char *name);

// get player pid by name
int sql_get_player_pid(const char *name);

// individual load functions (called by sql_load_player)
bool sql_load_player_status(P_char ch, int pid);
bool sql_load_player_skills(P_char ch);
bool sql_load_player_affects(P_char ch);
bool sql_load_player_items(P_char ch);
bool sql_load_player_witnesses(P_char ch);

// ============================================================================
// player delete
// ============================================================================

// delete player from db (for pwipe, etc)
bool sql_delete_player(int pid);
bool sql_delete_player_by_name(const char *name);

// ============================================================================
// account functions
// ============================================================================

// save account to db
bool sql_save_account(struct acct_entry *acc);

// load account from db by name
struct acct_entry *sql_load_account(const char *name);

// check if account exists
bool sql_account_exists(const char *name);

// link player to account (updates player_data.account_name)
bool sql_link_player_to_account(const char *account_name, int pid);

// ============================================================================
// locker functions
// ============================================================================

// save locker to db
// for personal locker: owner_pid set, owner_assoc_id = 0
// for guild locker: owner_pid = 0, owner_assoc_id set
bool sql_save_locker(P_char locker_ch, int owner_pid, int owner_assoc_id);

// load locker from db
// pass owner_pid for personal, owner_assoc_id for guild (other should be 0)
P_char sql_load_locker(int owner_pid, int owner_assoc_id);

// check if locker exists
bool sql_locker_exists(int owner_pid, int owner_assoc_id);

// delete locker
bool sql_delete_locker(int owner_pid, int owner_assoc_id);

// ============================================================================
// migration helpers
// ============================================================================

// migrate single player from pfile to db
// loads from pfile, saves to db, verifies
bool sql_migrate_player(const char *name);

// verify player data matches between pfile and db
bool sql_verify_player(const char *name);

// migrate all players from pfiles to db
// returns count of successfully migrated players
int sql_migrate_all_players(void);

// ============================================================================
// utility
// ============================================================================

// escape string for sql (wrapper around mysql_real_escape_string)
// caller must free returned string
char *sql_escape_string(const char *str);

// log sql error with context
void sql_player_error(const char *context, const char *query);

#endif // __NO_MYSQL__

#endif // __SQL_PLAYER_H_INCLUDED__
