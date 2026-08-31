#ifndef PLAYER_LOAD_MATERIALIZE_H
#define PLAYER_LOAD_MATERIALIZE_H

#include "player/player_load_repository.h"

struct char_data;
typedef struct char_data *P_char;

bool player_load_materialize(P_char character, const player_load_result &result);

#endif
