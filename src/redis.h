#ifndef __REDIS_H__
#define __REDIS_H__

#include <stdbool.h>
#include "structs.h"

extern bool redis_enabled;

bool redis_init(void);
void redis_cleanup(void);
bool redis_ping(void);

void mark_player_dirty(int pid);
void flush_dirty_players(void);
int get_dirty_player_count(void);
void event_flush_dirty_players(P_char ch, P_char victim, P_obj obj, void *data);

#endif
