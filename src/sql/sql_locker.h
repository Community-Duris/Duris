#ifndef DURIS_SQL_LOCKER_H_INCLUDED
#define DURIS_SQL_LOCKER_H_INCLUDED

#include "core/structs.h"

bool sql_save_locker(P_char locker_ch, int owner_pid, int owner_assoc_id);
P_char sql_load_locker(int owner_pid, int owner_assoc_id);
P_char sql_load_locker_by_name(const char *locker_name);
bool sql_locker_exists(int owner_pid, int owner_assoc_id);
bool sql_locker_exists_by_name(const char *locker_name);
bool sql_locker_owner_can_access(const char *locker_name, int owner_pid, int racewar);
bool sql_delete_locker(int owner_pid, int owner_assoc_id);
bool sql_delete_locker_by_name(const char *locker_name);

int sql_get_locker_id_by_name(const char *locker_name);
int sql_get_or_create_public_chest(int locker_id);
int sql_create_private_chest_hashed(int locker_id, const char *chest_name, const char *hash);
bool sql_delete_private_chest(int chest_id);
int sql_get_chest_id(int locker_id, const char *chest_name);
bool sql_set_chest_password_hash(int chest_id, const char *hash);
bool sql_get_chest_password_hash(int chest_id, char **hash);
bool sql_finish_chest_password(int chest_id, const char *expected, const char *upgrade);
int sql_count_private_chests(int locker_id);

#define CHEST_ACTION_OPEN 1
#define CHEST_ACTION_CLOSE 2
#define CHEST_ACTION_PUT 3
#define CHEST_ACTION_GET 4
#define CHEST_ACTION_FAIL 5

bool sql_log_chest_activity(int locker_id, int chest_id, const char *char_name, int action_type,
			    const char *item_short);
bool sql_save_private_chest_items(int locker_id, int chest_id, P_obj chest_obj);
void sql_load_private_chest_items(int locker_id, int chest_id, P_obj chest_obj);

#endif // DURIS_SQL_LOCKER_H_INCLUDED
