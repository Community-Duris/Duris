#ifndef PLAYER_SNAPSHOT_CAPTURE_H
#define PLAYER_SNAPSHOT_CAPTURE_H

#include "player_snapshot.h"

struct char_data;
typedef struct char_data *P_char;

player_snapshot_capture_result player_snapshot_capture(P_char ch, player_revision_t revision,
						       player_component_mask_t components,
						       int save_intent, int room_vnum,
						       player_snapshot *snapshot_out);

#endif
