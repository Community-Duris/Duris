// sql_player.h
// player save/load functions for mysql storage
// part of pfile-to-db migration

#ifndef __SQL_PLAYER_H_INCLUDED__
#define __SQL_PLAYER_H_INCLUDED__

#include "core/structs.h"

// ============================================================================
// player save functions
// ============================================================================

// master save function - saves entire player to db atomically
// type: save type (RENT_CAMPED, RENT_RENTED, etc from defines.h)
// room: room vnum to save
// returns true on success
bool sql_save_player(P_char ch, int type, int room);

// ============================================================================
// player load functions
// ============================================================================

// Legacy component readers retained for restoreCharOnly compatibility.
bool sql_load_player_status(P_char ch, int pid);
bool sql_load_player_skills(P_char ch);
bool sql_load_player_affects(P_char ch);
bool sql_load_player_shapechanges(P_char ch);

// Retired prototype-only loader fails closed; use ownership-aware materialization.
bool sql_load_player_pets(P_char ch);

#endif // __SQL_PLAYER_H_INCLUDED__
