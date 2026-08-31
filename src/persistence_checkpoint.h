#ifndef PERSISTENCE_CHECKPOINT_H
#define PERSISTENCE_CHECKPOINT_H

#include "persistence_observability.h"
#include "player/player_revision_state.h"
#include "structs.h"

void mark_player_dirty(int pid);
void mark_player_dirty_components(int pid, player_component_mask_t components);
void flush_dirty_players(void);
int get_dirty_player_count(void);
struct persistence_dirty_save_snapshot persistence_dirty_save_snapshot_copy(void);
void event_flush_dirty_players(P_char ch, P_char victim, P_obj obj, void *data);

#endif
