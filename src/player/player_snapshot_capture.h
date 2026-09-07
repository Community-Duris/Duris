#ifndef PLAYER_SNAPSHOT_CAPTURE_H
#define PLAYER_SNAPSHOT_CAPTURE_H

#include "player/player_snapshot.h"

struct char_data;
typedef struct char_data *P_char;
struct obj_data;
typedef struct obj_data *P_obj;

player_snapshot_capture_result player_snapshot_capture(P_char ch, player_revision_t revision,
						       player_component_mask_t components,
						       int save_intent, int room_vnum,
						       player_snapshot *snapshot_out);
player_snapshot_capture_result
player_item_snapshot_list_capture(P_char owner, bool equipment, bool inventory, bool omit_norent,
				  std::vector<player_item_snapshot> *items_out,
				  size_t *estimated_bytes_out);
player_snapshot_capture_result
player_item_snapshot_tree_capture(P_obj root, std::vector<player_item_snapshot> *items_out,
				  size_t *estimated_bytes_out);

// Capture an immutable death disposition without clearing cash or moving objects.
// wallet_pile is an unattached pile representing the complete current wallet.
player_snapshot_capture_result
player_death_snapshot_capture(P_char ch, P_obj corpse, P_obj wallet_pile,
			      const critical_operation_id &operation_id, player_revision_t revision,
			      int entry_room_vnum,
			      const std::vector<critical_operation_id> &unresolved_operations,
			      player_snapshot *snapshot_out);

#endif
