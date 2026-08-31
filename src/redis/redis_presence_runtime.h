#ifndef REDIS_PRESENCE_RUNTIME_H
#define REDIS_PRESENCE_RUNTIME_H

#include "structs.h"

bool redis_presence_runtime_enabled(void);
void redis_presence_runtime_set_enabled(bool enabled);
void redis_player_online(P_char character);
void redis_player_offline(P_char character);
void redis_clear_online_players(void);

#endif
