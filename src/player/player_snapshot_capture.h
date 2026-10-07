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

// Freeze all four literal strings without changing the live object's string mask.
// Every item in the selected tree must have a distinct nonzero, nonsentinel UID.
player_snapshot_capture_result
player_item_snapshot_tree_capture_literal(P_obj root, std::vector<player_item_snapshot> *items_out,
					  size_t *estimated_bytes_out);
// A nonzero root UID selects exactly one top-level carried tree. Both item
// components are required; siblings, equipment and pets retain ordinary capture.
// UID zero uses ordinary capture. Output is unchanged on any refusal.
player_snapshot_capture_result player_snapshot_capture_literal_inventory(
	P_char ch, player_revision_t revision, player_component_mask_t components, int save_intent,
	int room_vnum, uint64_t inventory_root_uid, player_snapshot *snapshot_out);

// Freeze all four strings for every selected top-level carried tree, including
// descendants; unrelated inventory, equipment and pets keep their original policy.
// The caller retains this exact root selection through save acknowledgement.
// Empty selection is ordinary capture; output is unchanged on refusal.
player_snapshot_capture_result player_snapshot_capture_literal_inventory_roots(
	P_char, player_revision_t, player_component_mask_t, int save_intent, int room_vnum,
	std::span<const uint64_t> inventory_roots, player_snapshot *);

// Capture an immutable death disposition without clearing cash or moving objects.
// wallet_pile is an unattached pile representing the complete current wallet.
player_snapshot_capture_result
player_death_snapshot_capture(P_char ch, P_obj corpse, P_obj wallet_pile,
			      const critical_operation_id &operation_id, player_revision_t revision,
			      int entry_room_vnum,
			      const std::vector<critical_operation_id> &unresolved_operations,
			      player_snapshot *snapshot_out);

#endif
